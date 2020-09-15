#include <System/tCommand.h>
#include <System/tScript.h>
#include <System/tFile.h>
using namespace tStd;
using namespace tSystem;


int main(int argc, char** argv)
{
	tPrintf("Woboq CompilerCommands Tool\n");

	tCommand::tOption config("config", "Specify the config file.", 'c', 1);
	tCommand::tParse(argc, argv);
	if (!config.IsPresent())
	{
		tCommand::tPrintUsage();
		return 0;
	}

	tString configFile = config.Arg1();
	tString outputFile = "compile_commands.json";
	tList<tStringItem> includePaths;
	tList<tStringItem> sourceFiles;
	tString compileCommand;

	//tPrintf("Config arg: %s\n", config.Arg1().Pod());
	tScriptReader script(configFile);
	for (tExpr exp = script.First(); exp.IsValid(); exp = exp.Next())
	{
		tString com = exp.Command().GetAtomString();
		tString arg1 = exp.Arg1().GetAtomString();
		if (com == "OutputFile")
		{
			outputFile = tGetAbsolutePath( tGetDir(configFile) + arg1 );
			tPrintf("OutputFile : %s\n", outputFile.Pod());
		}
		else if (com == "IncludePath")
		{
			tStringItem* incPath = new tStringItem( tGetLinuxPath( tGetAbsolutePath( tGetDir(configFile) + arg1 ) ) );
			includePaths.Append(incPath);
			tPrintf("IncludePath : %s\n", incPath->Pod());
		}
		else if (com == "SourceFile")
		{
			tStringItem* srcFile = new tStringItem( tGetLinuxPath( tGetAbsolutePath( tGetDir(configFile) + arg1 ) ) );
			sourceFiles.Append(srcFile);
			tPrintf("SourceFile : %s\n", srcFile->Pod());
		}
		else if (com == "CompileCommand")
		{
			compileCommand = arg1;
			tPrintf("CompileCommand : %s\n", compileCommand.Pod());
		}
	}

	if (sourceFiles.IsEmpty())
	{
		tPrintf("No source files to process.\n");
		return 0;
	}

	tFileHandle outFile = tOpenFile(outputFile, "wt");
	tfPrintf(outFile, "[\n");
	for (tStringItem* srcFile = sourceFiles.First(); srcFile; srcFile = srcFile->Next())
	{
		tfPrintf(outFile, "{\n");

		tString dir = tGetDir(*srcFile);
		dir[dir.Length()-1] = '\0';

		tfPrintf(outFile, "  \"directory\": \"%s\",\n", dir.Pod());

		tfPrintf(outFile, "  \"command\": \"%s ", compileCommand.Pod());
		for (tStringItem* incDir = includePaths.First(); incDir; incDir = incDir->Next())
			tfPrintf(outFile, "-I%s ", incDir->Pod());
		tfPrintf(outFile, " -o %s.o ", srcFile->Pod());
		tfPrintf(outFile, " -c %s\",\n", srcFile->Pod());

		tfPrintf(outFile, "  \"file\": \"%s\"\n", srcFile->Pod());

		if (srcFile->Next())
			tfPrintf(outFile, "},\n");
		else
			tfPrintf(outFile, "}\n");
	}

	tfPrintf(outFile, "]\n");
	tCloseFile(outFile);
	tPrintf("Generated: %s\n", outputFile.Pod());
	return 0;
}
