
#include"BFC/portable.h"
#include<iostream>
using namespace std;
using namespace ff;

int main()
{
    std::string exePath=ff::getExePath();
    cout<<exePath<<endl;
    copyFile("./data.txt","./ddir/data_copy.txt");
    cout<<"current dir="<<getCurrentDirectory()<<endl;
    cout<<"temp dir="<<getTempPath()<<endl;

    cout<<"list files:\n";
    std::vector<string> files;
    listFiles("./",files,true,false);
    for(auto &f : files)
        cout<<f<<' ';
    cout<<endl;

    cout<<"list dirs:\n";
    std::vector<string> dirs;
    listSubDirectories("./",dirs,false,false);
    for(auto &f : dirs)
        cout<<f<<' ';
    cout<<endl;

    
    return 0;
}

#include"BFC/portable_linux.cpp"
