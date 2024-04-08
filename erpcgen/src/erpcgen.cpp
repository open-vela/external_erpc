/*
 * Copyright (c) 2014-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "erpc_version.h"

#include "CGenerator.hpp"
#include "ErpcLexer.hpp"
#include "InterfaceDefinition.hpp"
#include "Logging.hpp"
#include "PythonGenerator.hpp"
#include "JavaGenerator.hpp"
#include "SearchPath.hpp"
#include "UniqueIdChecker.hpp"
#include "options.hpp"
#include "types/Program.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>

/*!
 * @brief Entry point for the tool.
 */
int main(int argc, char *argv[], char *envp[]);

namespace erpcgen {
using namespace std;
////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/*! The tool's name. */
const char k_toolName[] = "erpcgen";

/*! Current version number for the tool. */
const char k_version[] = ERPC_VERSION;

/*! Copyright string. */
const char k_copyright[] = "Copyright 2016-2021 NXP. All rights reserved.";

static const char *k_optionsDefinition[] = { "?|help",
                                             "V|version",
                                             "o:output <filePath>",
                                             "v|verbose",
                                             "I:path <filePath>",
                                             "g:generate <language>",
                                             "c:codec <codecType>",
                                             "p:package <packageName>",
                                             "d|delete",
                                             "a|add prefix",
                                             NULL };

/*! Help string. */
const char k_usageText[] =
    "\nOptions:\n\
  -?/--help                    Show this help\n\
  -V/--version                 Display tool version\n\
  -o/--output <filePath>       Set output directory path prefix\n\
  -v/--verbose                 Print extra detailed log information\n\
  -I/--path <filePath>         Add search path for imports\n\
  -g/--generate <language>     Select the output language (default is C)\n\
  -c/--codec <codecType>       Specify used codec type\n\
  -p/--package <packageName>   Java app package (com.example.app) (only for Java)\n\
  -d/--delete                  Delete erpc gen files\n\
  -a/--addprefix               Add client or server prefix for function name\n\
\n\
Available languages (use with -g option):\n\
  c    C/C++\n\
  py   Python\n\
  java Java\n\
\n\
Available codecs (use with --c option):\n\
  basic   BasicCodec\n\
\n";

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

/*!
 * @brief Class that encapsulates the erpcgen tool.
 *
 * A single global logger instance is created during object construction. It is
 * never freed because we need it up to the last possible minute, when an
 * exception could be thrown.
 */
class erpcgenTool
{
protected:
    enum class verbose_type_t
    {
        kWarning,
        kInfo,
        kDebug,
        kExtraDebug
    }; /*!< Types of verbose outputs from erpcgen application. */

    enum class languages_t
    {
        kCLanguage,
        kPythonLanguage,
        kJavaLanguage,
    }; /*!< Generated outputs format. */

    typedef vector<string> string_vector_t; /*!< Vector of positional arguments. */

    int m_argc;                           /*!< Number of command line arguments. */
    char **m_argv;                        /*!< String value for each command line argument. */
    StdoutLogger *m_logger;               /*!< Singleton logger instance. */
    verbose_type_t m_verboseType;         /*!< Which type of log is need to set (warning, info, debug). */
    const char *m_outputFilePath;         /*!< Path to the output file. */
    const char *m_ErpcFile;               /*!< ERPC file. */
    string_vector_t m_positionalArgs;     /*!< Positional arguments. */
    languages_t m_outputLanguage;         /*!< Output language we're generating. */
    InterfaceDefinition::codec_t m_codec; /*!< Used codec type. */
    string m_javaPackageName;             /*!< Used java package. */
    bool m_addPrefix;                     /*!< Flag hints for adding prefix for c/s func name. */
    bool m_isDelete;                      /*!< Flag hints for deleting erpc gen files. */

public:
    /*!
     * @brief Constructor.
     *
     * @param[in] argc Count of arguments in argv variable.
     * @param[in] argv Pointer to array of arguments.
     *
     * Creates the singleton logger instance.
     */
    erpcgenTool(int argc, char *argv[]) :
    m_argc(argc), m_argv(argv), m_logger(0), m_verboseType(verbose_type_t::kWarning), m_outputFilePath(NULL),
    m_ErpcFile(NULL), m_outputLanguage(languages_t::kCLanguage), m_codec(InterfaceDefinition::codec_t::kNotSpecified), m_addPrefix(false), m_isDelete(false)
    {
        // create logger instance
        m_logger = new StdoutLogger();
        m_logger->setFilterLevel(Logger::log_level_t::kWarning);
        Log::setLogger(m_logger);
    }

    /*!
     * @brief Destructor.
     */
    ~erpcgenTool() {}

    /*!
     * @brief Reads the command line options passed into the constructor.
     *
     * This method can return a return code to its caller, which will cause the
     * tool to exit immediately with that return code value. Normally, though, it
     * will return -1 to signal that the tool should continue to execute and
     * all options were processed successfully.
     *
     * The Options class is used to parse command line options. See
     * #k_optionsDefinition for the list of options and #k_usageText for the
     * descriptive help for each option.
     *
     * @retval -1 The options were processed successfully. Let the tool run normally.
     * @return A zero or positive result is a return code value that should be
     *      returned from the tool as it exits immediately.
     */
    int processOptions()
    {
        Options options(*m_argv, k_optionsDefinition);
        OptArgvIter iter(--m_argc, ++m_argv);

        // process command line options
        int optchar;
        const char *optarg;
        while ((optchar = options(iter, optarg)))
        {
            switch (optchar)
            {
                case '?':
                {
                    printUsage(options);
                    return 0;
                }

                case 'V':
                {
                    printf("%s %s\n%s\n", k_toolName, k_version, k_copyright);
                    return 0;
                }

                case 'o':
                {
                    m_outputFilePath = optarg;
                    break;
                }

                case 'v':
                {
                    if (m_verboseType != verbose_type_t::kExtraDebug)
                    {
                        m_verboseType = (verbose_type_t)(((int)m_verboseType) + 1);
                    }
                    break;
                }

                case 'I':
                {
                    PathSearcher::getGlobalSearcher().addSearchPath(optarg);
                    break;
                }

                case 'g':
                {
                    string lang = optarg;
                    if (lang == "c")
                    {
                        m_outputLanguage = languages_t::kCLanguage;
                    }
                    else if (lang == "py")
                    {
                        m_outputLanguage = languages_t::kPythonLanguage;
                    }
                    else if (lang == "java")
                    {
                        m_outputLanguage = languages_t::kJavaLanguage;
                    }
                    else
                    {
                        Log::error("error: unknown language %s", lang.c_str());
                        return 1;
                    }
                    break;
                }

                case 'c':
                {
                    string codec = optarg;
                    if (codec.compare("basic") == 0)
                    {
                        m_codec = InterfaceDefinition::codec_t::kBasicCodec;
                    }
                    else
                    {
                        Log::error("error: unknown codec type %s", codec.c_str());
                        return 1;
                    }
                    break;
                }

                case 'p':
                {
                    m_javaPackageName = optarg;
                    break;
                }

                case 'd':
                {
                    m_isDelete = true;
                    break;
                }

                case 'a':
                {
                    m_addPrefix = true;
                    break;
                }

                default:
                {
                    Log::error("error: unrecognized option\n\n");
                    printUsage(options);
                    return 0;
                }
            }
        }

        // handle positional args
        if (iter.index() < m_argc)
        {
            //            Log::debug("positional args:\n");
            int i;
            for (i = iter.index(); i < m_argc; ++i)
            {
                //                Log::debug("%d: %s\n", i - iter.index(), m_argv[i]);
                m_positionalArgs.push_back(m_argv[i]);
            }
        }

        // all is well
        return -1;
    }

    /*!
     * @brief Prints help for the tool.
     *
     * @param[in] options Options, which can be used.
     */
    void printUsage(const Options &options)
    {
        options.usage(cout, "files...");
        printf(k_usageText);
    }

    /*!
     * @brief Core of the tool.
     *
     * Calls processOptions() to handle command line options before performing the
     * real work the tool does.
     *
     * @retval 1 The functions wasn't processed successfully.
     * @retval 0 The function was processed successfully.
     *
     * @exception Log::error This function is called, when function wasn't
     *              processed successfully.
     * @exception runtime_error Thrown, when positional args is empty.
     */
    int run()
    {
        try
        {
            // read command line options
            int result;
            if ((result = processOptions()) != -1)
            {
                return result;
            }

            // set verbose logging
            setVerboseLogging();

            // check argument values
            checkArguments();
            if (!m_positionalArgs.size())
            {
                throw runtime_error("no input file provided");
            }

            m_ErpcFile = m_positionalArgs[0].c_str();
            if (!m_outputFilePath)
            {
                m_outputFilePath = "";
            }

            if (m_isDelete)
            {
                deleteGeneratedFiles();
                return 0;
            }

            // Parse and build definition model.
            InterfaceDefinition def;
            def.parse(m_ErpcFile);

            // Check for duplicate function IDs
            UniqueIdChecker uniqueIdCheck;
            uniqueIdCheck.makeIdsUnique(def);

            std::filesystem::path filePath(m_ErpcFile);
            def.setProgramInfo(filePath.filename().generic_string(), m_outputFilePath, m_codec);
            def.setAddPrefixFlag(m_addPrefix);

            switch (m_outputLanguage)
            {
                case languages_t::kCLanguage:
                {
                    CGenerator(&def).generate();
                    break;
                }
                case languages_t::kPythonLanguage:
                {
                    PythonGenerator(&def).generate();
                    break;
                }
                case languages_t::kJavaLanguage:
                {
                    // TODO: Check java package
                    JavaGenerator(&def, m_javaPackageName).generate();
                    break;
                }
            }
        }
        catch (exception &e)
        {
            Log::error("error: %s\n", e.what());
            return 1;
        }
        catch (...)
        {
            Log::error("error: unexpected exception\n");
            return 1;
        }

        return 0;
    }

    /*!
     * @brief Validate arguments that can be checked.
     *
     * @exception runtime_error Thrown if an argument value fails to pass validation.
     */
    void checkArguments()
    {
        //      if (m_outputFilePath == NULL)
        //      {
        //          throw runtime_error("no output file was specified");
        //      }
    }

    /*!
     * @brief Turns on verbose logging.
     */
    void setVerboseLogging()
    {
        // verbose only affects the INFO and DEBUG filter levels
        // if the user has selected quiet mode, it overrides verbose
        switch (m_verboseType)
        {
            case verbose_type_t::kWarning:
            {
                Log::getLogger()->setFilterLevel(Logger::log_level_t::kWarning);
                break;
            }
            case verbose_type_t::kInfo:
            {
                Log::getLogger()->setFilterLevel(Logger::log_level_t::kInfo);
                break;
            }
            case verbose_type_t::kDebug:
            {
                Log::getLogger()->setFilterLevel(Logger::log_level_t::kDebug);
                break;
            }
            case verbose_type_t::kExtraDebug:
            {
                Log::getLogger()->setFilterLevel(Logger::log_level_t::kDebug2);
                break;
            }
        }
    }

    /*!
     * @brief Delete erpc generated files.
     *
     * Delete erpc generated files include output paths.
     */
    void deleteGeneratedFiles()
    {
        char *programName = getProgramNameFromERPCFile();
        if (programName)
        {
            if (strcmp(m_outputFilePath, ""))
            {
                /* Directly remove folder */
                removeDirectory(m_outputFilePath);
            }
            else
            {
                char *outputDirName = getOutputDirFromFile();
                if (outputDirName)
                {
                    // if output path included in erpc file, directly delete the path
                    removeDirectory(outputDirName);
                    free(outputDirName);
                }
                else
                {
                    /* Remove related files */
                    removeFilesByGroupOrProgram(programName);
                    removeImportERPCFilesRecursive(m_ErpcFile, programName);
                }
            }
            free(programName);
        }
    }
private:
    /*!
     * @brief if the line is a comment sentence.
     *
     * @param[in] buffer the input line buffer.
     *
     * @retval true if the line is a comment sentence.
     * @retval false if the line is not a comment sentence.
     *
     */
    static bool isCommentLine(const char *buffer)
    {
        const char *end = buffer + strlen(buffer);

        // skip all space
        while ((*buffer == ' ') && (buffer < end))
        {
            buffer++;
        }

        if (buffer == end)
        {
            return false;
        }

        return (*buffer == '#') ? true : false;
    }

    /*!
     * @brief Get next target char offset.
     *
     * @param[in] start is the pointer to the begging of buffer.
     * @param[in] end is the pointer to the end of buffer.
     * @param[in] target is the char need to be found in buffer.
     *
     * Get the target char offset from start to end.
     *
     * @retval NULL if there is no target char in buffer.
     * @retval non-NULL if there is target char in buffer, and return the pointer.
     *
     */
    static char* getNextTargetCharPos(char *start, char *end, char target)
    {
        while (*start != target && (start < end))
        {
            start++;
        }

        if (start == end)
        {
            // can't find any target char
            return NULL;
        }

        return start;
    }

    /*!
     * @brief Get property value from line buffer.
     *
     * @param[in] strLine is the input line buffer.
     * @param[in] keyWord is the property name.
     *
     * Get the property value from line buffer indexed by keyWord.
     *
     * @retval NULL if there is no propery value in buffer.
     * @retval non-NULL if there is propety value in buffer, and return the value pointer.
     *
     */
    static char* getPropertyValue(char *strLine, const char *keyWord)
    {
        char *target = strstr(strLine, keyWord);
        char *end = strLine + strlen(strLine);
        char *nameFirstChar = NULL;

        if (target == NULL)
        {
            // can't find any program keyword in strLine
            return NULL;
        }

        target += strlen(keyWord);

        target = getNextTargetCharPos(target, end, '(');
        if (target == NULL)
        {
            return NULL;
        }

        target = getNextTargetCharPos(target, end, '"');
        if (target == NULL)
        {
            return NULL;
        }

        target++;
        nameFirstChar = target;

        nameFirstChar = getNextTargetCharPos(nameFirstChar, end, '"');
        if (nameFirstChar == NULL)
        {
            return NULL;
        }

        *nameFirstChar = 0;

        return strdup(target);
    }

    /*!
     * @brief Get property from erpcFile.
     *
     * @param[in] func User defined function for getting property.
     *
     * Get property from erpc file through user defined function
     *
     * @retval NULL if there is no property in buffer.
     * @retval non-NULL if there is property in buffer and return the property pointer.
     *
     */
    char* getPropertyFromERPCFile(char* (*func)(char *))
    {
        FILE *fd;
        char buffer[512];
        char *target = NULL;

        fd = fopen(m_ErpcFile, "r");
        if (fd == NULL)
        {
            return NULL;
        }

        while (NULL != fgets(buffer, sizeof(buffer), fd))
        {
            if (feof(fd))
                break;
            if (isCommentLine(buffer))
            {
                continue;
            }
            else if (target = func(buffer))
            {
                break;
            }
        }

        fclose(fd);
        return target;
    }

    /*!
     * @brief get program name from line buffer
     *
     * @param[in] strLine is the input line buffer.
     *
     * Parse program name from line buffer, get return.
     *
     * @retval NULL if no valid program name.
     * @retval non-NULL if there is valid program name.
     *
     */
    static char* getProgramName(char *strLine)
    {
        char *target = strstr(strLine, "program ");
        char *end = strLine + strlen(strLine);
        char *nameFirstChar = NULL;

        if (target == NULL)
        {
            // can't find any program keyword in strLine
            return NULL;
        }

        target += strlen("program ");

        while (!isalpha(*target) && (target < end))
        {
            target++;
        }

        if (target == end)
        {
            // can't find any alphabet or number after program
            return NULL;
        }

        nameFirstChar = target;
        while ((isalnum(*nameFirstChar) || (*nameFirstChar == '-') || (*nameFirstChar == '_')) &&
               (nameFirstChar < end) && (*nameFirstChar != '#'))
        {
            nameFirstChar++;
        }

        *nameFirstChar = 0;

        return strdup(target);
    }

    /*!
     * @brief Get program name from erpcFile
     *
     * Get the program name from the head erpc file, import files don't have program keyword.
     *
     * @retval NULL if there is no valid program name.
     * @retval non-NULL if there is a valid program name.
     *
     */
    char *getProgramNameFromERPCFile()
    {
        return getPropertyFromERPCFile(getProgramName);
    }

    /*!
     * @brief Get group name from line buffer.
     *
     * @param[in] strLine is the input buffer.
     *
     * @retval NULL if there is no valid group name.
     * @retval non-NULL if there is a valid group name and return the pointer.
     *
     */
    static char* getGroupName(char *strLine)
    {
        return getPropertyValue(strLine, "@group");
    }

    /*!
     * @brief Get output dir name from line buffer.
     *
     * @param[in] strLine is the input buffer.
     *
     * @retval NULL if there is no valid output_dir name.
     * @retval non-NULL if there is a valid output_dir name and return the pointer.
     *
     */
    static char* getOutputDirName(char *strLine)
    {
        return getPropertyValue(strLine, "@output_dir");
    }

    /*!
     * @brief Get output directory name from erpcFile
     *
     * Get the out dir name from the head erpc file, import files don't have output_dir keyword.
     *
     * @retval NULL if there is no valid output_dir name.
     * @retval non-NULL if there is a valid output_dir name.
     *
     */
    char* getOutputDirFromFile()
    {
        return getPropertyFromERPCFile(getOutputDirName);
    }

    /*!
     * @brief Remove the full directory indicated by path.
     *
     * @param[in] path is the input path.
     *
     * Delete the whole directory including the sub dir files.
     *
     */
    static void removeDirectory(const char *path)
    {
        struct dirent *entry = NULL;
        DIR *dir = NULL;

        dir = opendir(path);
        if (dir == NULL)
        {
            return;
        }
        while (entry = readdir(dir))
        {
            DIR *sub_dir = NULL;
            FILE *file = NULL;
            char abs_path[PATH_MAX+1] = {0};
            if(*(entry->d_name) != '.')
            {
                sprintf(abs_path, "%s/%s", path, entry->d_name);
                if(sub_dir = opendir(abs_path))
                {
                    closedir(sub_dir);
                    removeDirectory(abs_path);
                }
                else
                {
                    if(file = fopen(abs_path, "r"))
                    {
                        fclose(file);
                        remove(abs_path);
                    }
                }
            }
        }
        remove(path);
    }

    /*!
     * @brief Remove the generated files by program name or group name.
     *
     * @param[in] pgname is the program or group name.
     *
     * Delete the generated files under current path.
     *
     */
    void removeFilesByGroupOrProgram(const char *pgname)
    {
         // c language will gen xxx_server.cpp, c_xxx_server.cpp, xxx_interface.cpp xxx_server.hpp
         // c_xxx_server.h xxx_interface.hpp xxx_common.h xxx_common.hpp xxx_client.cpp
         // c_xxx_client.cpp xxx_client.hpp c_xxx_client.h (xxx is interfaceName)
        if (m_outputLanguage == languages_t::kCLanguage) {
            char fname[PATH_MAX+1] = {0};
            const char *c_gen_file_prefix_str [] = {
                "",
                "c_",
                "",
                "",
                "c_",
                "",
                "",
                "",
                "",
                "c_",
                "",
                "c_",
            };
            const char *c_gen_file_suffix_str [] = {
                "_server.cpp",
                "_server.cpp",
                "_interface.cpp",
                "_server.hpp",
                "_server.h",
                "_interface.hpp",
                "_common.h",
                "_common.hpp",
                "_client.cpp",
                "_client.cpp",
                "_client.hpp",
                "_client.h",
            };

            for (size_t i = 0; i < (sizeof(c_gen_file_suffix_str) / sizeof(const char *)); ++i) {
                strcpy(fname, c_gen_file_prefix_str[i]);
                strcat(fname, pgname);
                strcat(fname, c_gen_file_suffix_str[i]);
                remove(fname);
            }
        }
        // python only generate xxx folder and __init__.py (xxx is interfaceName)
        else if (m_outputLanguage == languages_t::kPythonLanguage)
        {
            removeDirectory(pgname);
            remove("__init__.py");
        }
        // Java only generate xxx folder (xxx is interfaceName)
        else if (m_outputLanguage == languages_t::kJavaLanguage)
        {
            removeDirectory(pgname);
        }
    }

    /*!
     * @brief Get the imported file path.
     *
     * @param[in] strLine is the line buffer.
     *
     * Get the imported sub erpc file path by parsing erpc file.
     *
     * @retval NULL if there is no import file.
     * @retval non-NULL if there is valid import file.
     *
     */
    static char* getImportFilePath(char *strLine)
    {
        char *target = strstr(strLine, "import ");
        char *end = strLine + strlen(strLine);
        char *nameFirstChar = NULL;

        if (target == NULL)
        {
            // can't find any import keyword in strLine
            return NULL;
        }

        target += strlen("import ");

        target = getNextTargetCharPos(target, end, '"');
        if (target == NULL)
        {
            return NULL;
        }

        target++;
        nameFirstChar = target;

        nameFirstChar = getNextTargetCharPos(nameFirstChar, end, '"');
        if (nameFirstChar == NULL)
        {
            // no last "
            return NULL;
        }

        *nameFirstChar = 0;

        return strdup(target);
    }

    /*!
     * @brief Remove the erpc generated files which are import by root erpc file.
     *
     * @param[in] rootName is the erpc file name.
     * @param[in] programName is the program name given by root erpc file.
     *
     * Delete the files recursivly generated by imported erpc files.
     *
     */
    void removeImportERPCFilesRecursive(const char *rootName, const char *programName)
    {
        FILE *fd;
        char buffer[512];
        char *target = NULL;

        fd = fopen(rootName, "r");
        if (fd == NULL)
        {
            return;
        }

        while (NULL != fgets(buffer, sizeof(buffer), fd))
        {
            if (feof(fd))
                break;
            if (target = getGroupName(buffer))
            {
                strcpy(buffer, programName);
                strcat(buffer, "_");
                strcat(buffer, target);
                removeFilesByGroupOrProgram(buffer);
                free(target);
            }
            else if (target = getImportFilePath(buffer))
            {
                removeImportERPCFilesRecursive(target, programName);
                free(target);
            }
        }

        fclose(fd);
    }
};

} // namespace erpcgen

/*!
 * @brief Main application entry point.
 *
 * Creates a tool instance and lets it take over.
 */
int main(int argc, char *argv[], char *envp[])
{
    (void)envp;
    try
    {
        return erpcgen::erpcgenTool(argc, argv).run();
    }
    catch (...)
    {
        Log::error("error: unexpected exception\n");
        return 1;
    }

    return 0;
}
