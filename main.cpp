#include "src/ConfigReader.h"
#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{

    if(argc !=4)
        printf("Error. Provide 3 argument: Wows directory, mod folder and output directory\n");
    ConfigReader reader;
    Generator gen;
    QString wowsdir = argv[1];
    QFileInfo fi(wowsdir+"/res/banks/mod.xml");
    if(!fi.exists())
    {
        printf("Error. Can't found mod.xml in Wows directory.\n");
        return 1;
    }
    reader.openConfig("F:\\Games\\World_of_Warships_RU\\res\\banks\\mod.xml");
    reader.parse();
    OldConfig old;
    QString modFolder = argv[2];
    if(!QFileInfo(modFolder).exists())
    {
        printf("Error. Mod directory was not found.\n");
        return 1;
    }
    old.openModFolder(modFolder, reader, gen);
    old.mergeAndAlignAll(reader,gen);
    QString outputDir = argv[3];
    if(!QFileInfo(outputDir).exists())
    {
        printf("Error. Output directory was not found.\n");
        return 1;
    }
    gen.outputDir = outputDir;
    gen.generate(false, old.modName);
    return 0;
}
