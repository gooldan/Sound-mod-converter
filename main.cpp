#include "src/ConfigReader.h"
#include <iostream>

using namespace std;
int wmain(int argc, wchar_t **argv)
{

    printf("Welcome to the sound mod converter ver. 1.1. Starting converting process.\n");
    if(argc !=4)
        printf("Error. Provide 3 argument: Wows directory, mod folder and output directory\n");

    ConfigReader reader;
    Generator gen;
    QString wowsdir = QString::fromWCharArray(argv[1]);
    QFileInfo fi(wowsdir+"/res/banks/mod.xml");
    if(!fi.exists())
    {
        printf("Error. Can't found mod.xml in Wows directory.\n");
        return 1;
    }
    if(reader.openConfig(fi.absoluteFilePath()) != ConfigReader::OK)
    {
        printf("Error open config\n");
        return 1;
    }
    if(reader.parse() != ConfigReader::OK)
    {
        printf("Error parse config\n");
        return 1;
    }
    OldConfig old;
    QString modFolder = QString::fromWCharArray(argv[2]);
    if(!QFileInfo(modFolder).exists())
    {
        printf("Error. Mod directory was not found.\n");
        return 1;
    }
    old.openModFolder(modFolder, reader, gen);
    old.mergeAndAlignAll(reader,gen);
    QString outputDir = QString::fromWCharArray(argv[3]);
    if(!QFileInfo(outputDir).exists())
    {
        printf("Error. Output directory was not found.\n");
        return 1;
    }
    gen.outputDir = outputDir;
    gen.generate(false, old.modName);
    return 0;
}
