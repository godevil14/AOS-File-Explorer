#include <bits/stdc++.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
using namespace std;

#define clr cout<<"\033[H\033[J";
#define resetcursor cout<<"\033[1;1H";
#define setwinsize cout << "\e[8;50;220t";

struct termios orig_termios;

stack <string> leftstack;
stack <string> rightstack;
vector<string> currdir;
char root[1024];
char currpath[1024];
string currentpath;
string rootpath;
string command="";
vector<string> command_words;
int tracker=1;
int currdirtracker=0;
int trackerforkeyl=0;
int displaycontent;


void printdirectory(const char* p)
{
 clr;
 cout<<"\033[20;1H";cout<<"      :::::NORMAL MODE:::::";
 resetcursor;
 currdir.clear();
 struct dirent *myfile;
 struct stat fileStat;
 struct passwd *pw; 
 struct group *gp;
 char *c;
 int i;
 DIR *mydir = opendir(p);
 chdir(p);
 while ((myfile = readdir(mydir)) != NULL)
   {
   string str(myfile->d_name);
   currdir.push_back(str);
   }
   sort(currdir.begin(), currdir.end());
   int currdirsize = currdir.size();
   displaycontent = min(15,currdirsize); //display 15 files at a time
   int begin;int end;
   if(trackerforkeyl<=14)
   {begin=0;
    end=displaycontent-1;}
   else {begin=trackerforkeyl-14;end=trackerforkeyl;}
   for(int s=begin; s<=end;s++)
   {
    string str= currdir[s];
    if(str.length()>25)
    {
     for(i=0;i<=21;i++)
      cout<<str[i];
     cout<<"...";
    }
    else   cout<<left<<setw(25)<<str;
       cout<<"   ";
    const char *st = str.c_str();
    stat(st,&fileStat);  
        
    if(S_ISDIR(fileStat.st_mode)) 
       cout<<"d";
    else cout<<"-";
    if(fileStat.st_mode & S_IRUSR) 
       cout<<"r";
    else cout<<"-";
    if(fileStat.st_mode & S_IWUSR) 
       cout<<"w"; 
    else cout<<"-";
    if(fileStat.st_mode & S_IXUSR) 
       cout<<"x";
    else cout<<"-";
    if(fileStat.st_mode & S_IRGRP)
       cout<<"r";
    else cout<<"-";
    if(fileStat.st_mode & S_IWGRP)
       cout<<"w";
    else cout<<"-";
    if(fileStat.st_mode & S_IXGRP)
       cout<<"x";
    else cout<<"-";
    if(fileStat.st_mode & S_IROTH)
       cout<<"r";
    else cout<<"-";
    if(fileStat.st_mode & S_IWOTH)
       cout<<"w";
    else cout<<"-";
    if(fileStat.st_mode & S_IXOTH)
       cout<<"x";
    else cout<<"-"; 
    cout<<"   ";
    long int size = fileStat.st_size;
    float sizetrue;
    if(size>=1024 && size<(1024*1024))
    { sizetrue = size/1024.0;
      cout<<left<<setw(7)<<fixed<<setprecision(2)<<sizetrue<<" KB ";
    }
    else if(size>=(1024*1024) && size<(1024*1024*1024))
    {sizetrue = size/(1024.0*1024.0);
      cout<<left<<setw(7)<<fixed<<setprecision(2)<<sizetrue<<" MB ";}
    else if(size>=(1024*1024*1024))
    {sizetrue = size/(1024.0*1024.0*1024.0);
      cout<<left<<setw(7)<<fixed<<setprecision(2)<<sizetrue<<" GB ";}
    else {cout<<left<<setw(7)<<size<<"  B ";}
    cout<<"   ";
    pw=getpwuid(fileStat.st_uid);
    cout<<(pw->pw_name);
    cout<<"   ";
    gp=getgrgid(fileStat.st_gid);
    cout<<(gp->gr_name);
    cout<<"   ";
    c=ctime(&fileStat.st_mtime);
    for(i=0;i<=15;i++)
    cout<<c[i]; 
    cout<<endl;
    
    }
  
    closedir(mydir);   
}


int cursorpos=3;
void movecursor(int cursorpos)
{
 cout<<"\033[18;"<<cursorpos<<"H";
}

bool checkpath(string givenpath) //this is more of a check directory
{
 char path[512];
 int k;
 for(k=0;k<givenpath.length();k++)
 { path[k]=givenpath[k];
 }
 path[k]='\0';
 struct stat fileStat;
 stat(path,&fileStat);
 if (S_ISDIR(fileStat.st_mode)) // if path points to a valid directory
 {return true;}
 else {return false;} 
}

bool checkifpresent(string givenpath) // used in move or copy to see if the file or folder provided is valid
{
 char path[512];
 int k;
 for(k=0;k<givenpath.length();k++)
 { path[k]=givenpath[k];
 }
 path[k]='\0';
 struct stat fileStat;
 stat(path,&fileStat);
 if (S_ISDIR(fileStat.st_mode) || S_ISREG(fileStat.st_mode))
 {return true;}
 else {return false;} 
}


string modifypath(string temp)
{
 string mpath="";
 if(temp[0]=='.')
   {int length=temp.length()-1; mpath=currentpath+temp.substr(1,length);}
 else if(temp[0]=='~')
   {int length=temp.length()-1; mpath=rootpath+temp.substr(1,length);}
 else if(temp[0]=='/')
   mpath=temp;
 return mpath;
}

void renam(vector<string> comm)
{
 if(comm.size()==3)
 {string old= comm[1];
  string newfile= comm[2];
  rename(old.c_str(),newfile.c_str());
 }
}

bool searchfunction(string rn, string searchcontent)
{
 string currentdir=rn;
 struct dirent* myff;
 struct stat fileStat;
 DIR* mydir= opendir(currentdir.c_str());
 if(mydir==NULL)
  return false;
 while((myff = readdir(mydir)) != NULL)
 {
  string str(myff->d_name);
  if(str=="."|| str=="..")
  continue;
  else { string searchcontentpath = currentdir+"/"+str;
         stat(searchcontentpath.c_str(),&fileStat);
         if (S_ISDIR(fileStat.st_mode)) // if it is a directory
         {
          if(searchcontent==str)
               return true;
          else{
               bool recursive=searchfunction(searchcontentpath,searchcontent);
               if(recursive == true)
                  return true;
              }
         }
         else {  // it is a file
               if(searchcontent==str)
                 return true;
              } 
        
       }
 }
 closedir(mydir);
 return false; 
}

void deletedirectory(string directorypath)
{
string dirpath=directorypath;
 struct dirent* myff;
 struct stat fileStat;
 DIR* mydir= opendir(directorypath.c_str());
 while((myff = readdir(mydir)) != NULL)
 {
  string str(myff->d_name);
  if(str=="."|| str=="..")
     continue;
  else {
        string recursivepath = dirpath+"/"+str;
        stat(recursivepath.c_str(),&fileStat);
        if (S_ISDIR(fileStat.st_mode)) // if it is a directory
        {
         deletedirectory(recursivepath);
        }
        else {  // it is a file
               unlink(recursivepath.c_str());
             } 

        }
  }
  closedir(mydir);
  rmdir(directorypath.c_str());
}

void copyfile(string sourcepath, string copypath)
{ 
 int originalfile=open(sourcepath.c_str(),O_RDONLY);
 struct stat sourcefilestat;
 stat(sourcepath.c_str(),&sourcefilestat);
 int copiedfile=open(copypath.c_str(),O_WRONLY|O_CREAT, sourcefilestat.st_mode);
 char buffer[512]; int bytes;
 while((bytes=read(originalfile,buffer,sizeof(buffer))) != 0)
   {write(copiedfile,buffer,bytes);}
 chown(copypath.c_str(),sourcefilestat.st_uid,sourcefilestat.st_gid);
}

void copydirectory(string sourcepath, string copypath)
{
 
 DIR *mydir = opendir(sourcepath.c_str());
 chdir(sourcepath.c_str());
 mkdir(copypath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
 struct dirent *myfile;
 while((myfile = readdir(mydir)) != NULL)
 {
  string str(myfile->d_name);
  if(str=="."|| str=="..")
  continue;
  else { string insidedirectorypath = sourcepath+"/"+str;
         string insidedestdirectorypath = copypath+"/"+str;
         struct stat fileStat;
         stat(insidedirectorypath.c_str(),&fileStat);
         if (S_ISDIR(fileStat.st_mode)) // if it is a directory
         {
          copydirectory(insidedirectorypath,insidedestdirectorypath);
         }
         else {  
                copyfile(insidedirectorypath,insidedestdirectorypath);
              } 
        
       }
 }
 closedir(mydir); 
}

void movefile(string sourcepath, string movepath)
{
 copyfile(sourcepath,movepath);
 unlink(sourcepath.c_str());
}

void movedirectory(string sourcepath, string movepath)
{
 copydirectory(sourcepath,movepath);
 deletedirectory(sourcepath);
}


void commandmode()
{
 cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
 cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
 movecursor(3);
 char keypress[3];
 
 while(true)
 { 
  fflush(stdout);
  for(int i=0;i<3;i++)
   keypress[i]='0';
  read(STDIN_FILENO, keypress, 3);
  if(keypress[0]=='q' && cursorpos==3)
      {clr;
      resetcursor;
      cout<<"                   :::: EXITING File Explorer::::    "<<endl;
      usleep(800000);
      clr;
      exit(0);}
  else if(keypress[0]==27 && keypress[1]=='0' && keypress[2]=='0') // esc key pressed (back to normal mode)
   { cout<<"\x1b[2K"; //clearing out the symbol before command typing space
     cout<<"\033[19;1H";cout<<"\x1b[2K";//to clear search result TRUE or FALSE line if switched to command mode right after search query
     cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::NORMAL MODE:::::";
     resetcursor;
     break;
   }
  else if(keypress[0]==10) //enter is pressed
    { 
      cout<<"\033[19;1H";cout<<"\x1b[2K";//to clear search result TRUE or FALSE line when next time enter is pressed
      cursorpos=3;//<<moving back to command line
      movecursor(cursorpos);
     if(command=="")
       {continue;}
     else{
          cursorpos=3;
          movecursor(cursorpos);
          cout<<"\x1b[0K";
          string temp="";
          for(int i=0;i<command.length();i++)
          {
            if(command[i]==' ')
              {command_words.push_back(temp);
               temp="";
               continue;}
             else{ temp=temp+command[i];}
          }
          command_words.push_back(temp);
          
          if(command_words.size()>=2) //to check a necessary but not sufficient condition for valid command
            {
             string request = command_words[0];
             if(request=="copy")
             {
              if(command_words.size()>=3)
              {
               string pathcheck=modifypath(command_words[command_words.size()-1]);
               if(checkpath(pathcheck)) //go ahead with copy operation only if path provided is valid
               {           
               int contentsize=command_words.size()-2;
               for(int i=1;i<=contentsize;i++)
               {
                string readcontent=command_words[i];
                string sourcepath= currentpath+"/"+readcontent;
                if(checkifpresent(sourcepath))  //go ahead with copying current content only if it is present in the first place
                {string destinationpath= modifypath(command_words[command_words.size()-1]);
                destinationpath=destinationpath+"/"+readcontent;
                struct stat fileStat;
                stat(sourcepath.c_str(), &fileStat);
                if(S_ISDIR(fileStat.st_mode))
                  {copydirectory(sourcepath,destinationpath);}
                  else {copyfile(sourcepath,destinationpath);} }
               }
               }
              }
             }
             else if(request=="move")
             {
              if(command_words.size()>=3)
              { 
               string pathcheck=modifypath(command_words[command_words.size()-1]);
               if(checkpath(pathcheck)) //go ahead with move operation only if path provided is valid
               {
               int contentsize=command_words.size()-2;
               for(int i=1;i<=contentsize;i++)
               {
                string readcontent=command_words[i];
                string sourcepath= currentpath+"/"+readcontent;
                if(checkifpresent(sourcepath)) //go ahead with moving current content only if it is present in the first place
                {string destinationpath= modifypath(command_words[command_words.size()-1]);
                destinationpath=destinationpath+"/"+readcontent;
                struct stat fileStat;
                stat(sourcepath.c_str(), &fileStat);
                if(S_ISDIR(fileStat.st_mode))
                  {movedirectory(sourcepath,destinationpath);}
                  else {movefile(sourcepath,destinationpath);}  }
               }
               currdirtracker=0;  //<<< from this line , in case move operation done in same directory, refresh the visible directory by reloading it in the terminal so that update can be seen
               trackerforkeyl=0;
               tracker=1;
               printdirectory(currpath);
               cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
               cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
               movecursor(3);
               }
              }
             }
             else if(request=="rename")
             { renam(command_words);
               currdirtracker=0;
               trackerforkeyl=0;
               tracker=1;
               printdirectory(currpath);
               cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
               cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
               movecursor(3);
             }
             else if(request=="create_file")
             {
              if(command_words.size()==3)
              {
               string destpath= modifypath(command_words[2]);
               destpath+="/"+command_words[1];
               open(destpath.c_str(),O_RDWR|O_CREAT,S_IRWXU|S_IRGRP|S_IROTH);
               currdirtracker=0;  //<<< from this line , in case file created in same directory, refresh the visible directory by reloading it in the terminal so that new file can be seen
               trackerforkeyl=0;
               tracker=1;
               printdirectory(currpath);
               cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
               cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
               movecursor(3);
              }
             }
             else if(request=="create_dir")
             {
              if(command_words.size()==3)
              {
               string destpath= modifypath(command_words[2]);
               destpath+="/"+command_words[1];           
               mkdir(destpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
               currdirtracker=0;  //<<< from this line , in case dir created in same directory, refresh the visible directory by reloading it in the terminal so that new dir can be seen
               trackerforkeyl=0;
               tracker=1;
               printdirectory(currpath);
               cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
               cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
               movecursor(3);
             }
             }
             else if(request=="delete_file")
             {
              if(command_words.size()==2)
              {
               string path= modifypath(command_words[1]);
               unlink(path.c_str());
               currdirtracker=0;  //<<< from this line , in case file deleted in same directory, refresh the visible directory by reloading it in the terminal so that update can be seen
               trackerforkeyl=0;
               tracker=1;
               printdirectory(currpath);
               cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
               cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
               movecursor(3);
              }
             }
             else if(request=="delete_dir")
             {
              if(command_words.size()==2)
              {
               string path= modifypath(command_words[1]);
               if(checkpath(path))
               {
                deletedirectory(path);
                currdirtracker=0;  //<<< from this line , in case directory deleted in same directory, refresh the visible directory by reloading it in the terminal so that update can be seen
                trackerforkeyl=0;
                tracker=1;
                printdirectory(currpath);
                cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
                cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
                movecursor(3);
               }
              }
             }
             else if(request=="goto")
             {
               if(command_words.size()==2)
               {
                string path= modifypath(command_words[1]);
                if(checkpath(path))
                 {
                 leftstack.push(currentpath);
                 currentpath=path;
                 int i;
                 for(i=0;i<currentpath.length();i++)
                 { currpath[i]=currentpath[i];
                 }
                 currpath[i]='\0';
                 currdirtracker=0;
                 trackerforkeyl=0;
                 tracker=1;
                 printdirectory(currpath);
                 cout<<"\033[20;1H";cout<<"\x1b[2K"; cout<<"      :::::COMMAND MODE:::::";
                 cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
                 movecursor(3);
                 }
               }
             }
             else if(request=="search")
             {
              if(command_words.size()==2)
              {
               bool search_result=searchfunction(currentpath,command_words[1]);
               if(search_result)
               {
               cout<<"\033[19;1H"<<"  SEARCH RESULT : TRUE";
               cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
               movecursor(3);
               }
               else {
                     cout<<"\033[19;1H"<<"  SEARCH RESULT : FALSE";
                     cout<<"\033[18;1H"<<">>"; //reset cursor at line 18 to take command
                     movecursor(3);
                    }
              }
             }
            }
         }
         command.clear(); command_words.clear(); //clear out command string and associated vector after enter is pressed and command is processed
    }
  else if(keypress[0]==127) // backspace is pressed
   {
    if(cursorpos>3)
    {command.pop_back();
    cursorpos--;
    movecursor(cursorpos);cout<<"\x1b[0K";
    }
   }
  else if(keypress[0]==27 && keypress[1]=='[' && ( keypress[2]=='A' || keypress[2]=='B' ||keypress[2]=='C' ||keypress[2]=='D'  ) )
    continue;
  else {
        command.push_back(keypress[0]);
        cout<<keypress[0];
        cursorpos++;
        movecursor(cursorpos);
       }

 }

}




void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void EnableNormalMode() 
{
  clr;
  setwinsize;
  
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  
  getcwd(root,1024);
  strcpy(currpath,root);
  rootpath=string(root);
  currentpath=rootpath;
  printdirectory(root);
  resetcursor;
  fflush(stdout);
   char key[3];
  while(1)
  {
   fflush(stdout);
   for(int i=0;i<3;i++)
     key[i]='0';
   read(STDIN_FILENO, key, 3);
   
   if(key[2]=='A') //Up Arrow key pressed
   {
    if(tracker==1)
     continue;
    else {tracker--;
          currdirtracker--;
          cout<<"\033["<<tracker<<";1H";
         }
   }
   else if(key[2]=='B') //Down Arrow key pressed 
   {
    if(tracker==displaycontent)
     continue;
    else {tracker++;
          currdirtracker++;
          cout<<"\033["<<tracker<<";1H";
         }
   }
   else if(key[0]=='l')
   {
    if( tracker==15 && (currdirtracker)<(currdir.size()-1))
      {
       currdirtracker++;
       trackerforkeyl=currdirtracker;
       printdirectory(currpath);
       cout<<"\033["<<tracker<<";1H";
      }
   }
   else if(key[0]=='k')
   {
    if( tracker==1 && currdirtracker>0)
      {
       currdirtracker--;
       trackerforkeyl--;
       printdirectory(currpath);
       cout<<"\033["<<tracker<<";1H";
      }
   }
   else if(key[0]==10) //Enter key is pressed
   {
    if(currdir[currdirtracker]==".")
     continue; 
    else if(currdir[currdirtracker]=="..")
    { currentpath=string(currpath);
     if(currentpath!=rootpath)
     { leftstack.push(currentpath);
      int pathsize = currentpath.length();
      while(currentpath[pathsize-1]!='/')
      {
       currentpath.pop_back();
       pathsize--;
      }
      currentpath.pop_back();
      int i;
      for(i=0;i<currentpath.length();i++)
      { currpath[i]=currentpath[i];
      }
      currpath[i]='\0';
      currdirtracker=0;
      trackerforkeyl=0;
      tracker=1;
      printdirectory(currpath);
      resetcursor;
     }
    }  
    else{ string currentpathforleftstack=currentpath; //using this string variable to push in left stack in case next entering location is a directory
          currentpath=string(currpath)+"/"+currdir[currdirtracker];
          char path[512];
          int k;
           for(k=0;k<currentpath.length();k++)
           { path[k]=currentpath[k];
           }
           path[k]='\0';
          struct stat fileStat;
          stat(path,&fileStat);
          if ((fileStat.st_mode & S_IFMT)== S_IFDIR)
          { 
           leftstack.push(currentpathforleftstack);
           int i;
           for(i=0;i<currentpath.length();i++)
           { currpath[i]=currentpath[i];
           }
           currpath[i]='\0';
           currdirtracker=0;
           trackerforkeyl=0;
           tracker=1;
           printdirectory(currpath);
           resetcursor;
          }
          else
          { leftstack.push(currentpathforleftstack);
            
           pid_t pid = fork();
           if (pid == 0) {
           execl("/usr/bin/xdg-open", "xdg-open", path, (char *)0);
           exit(1);} 
          }
        }
   } 
   else if(key[0]=='h')
   { leftstack.push(currentpath);
    currentpath=rootpath;
    int i;
    for(i=0;i<currentpath.length();i++)
    { currpath[i]=currentpath[i];
    }
    currpath[i]='\0';
    currdirtracker=0;
    trackerforkeyl=0;
    tracker=1;
    printdirectory(currpath);
    resetcursor;
   }
   else if(key[0]==127) //backspace key
   { leftstack.push(currentpath);
     currentpath=string(currpath);
     if(currentpath!=rootpath)
     {
      int pathsize = currentpath.length();
      while(currentpath[pathsize-1]!='/')
      {
       currentpath.pop_back();
       pathsize--;
      }
      currentpath.pop_back();
      int i;
      for(i=0;i<currentpath.length();i++)
      { currpath[i]=currentpath[i];
      }
      currpath[i]='\0';
      currdirtracker=0;
      trackerforkeyl=0;
      tracker=1;
      printdirectory(currpath);
      resetcursor;
     }   
   }
   else if(key[0]=='q')
   { clr;
     resetcursor;
     cout<<"                   :::: EXITING File Explorer::::    "<<endl;
     usleep(800000);
     clr;
     break;
   }
   else if(key[2]=='D') // left arrow key is pressed
   {
    if(!(leftstack.empty()))
     { 
     if(checkpath(currentpath)) //in case last thing clicked was a file, it'd be in current path..let's not push it, first modify it
      rightstack.push(currentpath);
     string temp=leftstack.top();
     currentpath=temp;
     leftstack.pop();
     int i;
     for(i=0;i<currentpath.length();i++)
     { currpath[i]=currentpath[i];
     }
     currpath[i]='\0';
     currdirtracker=0;
     trackerforkeyl=0;
     tracker=1;
     printdirectory(currpath);
     resetcursor;
     }
   }
   else if(key[2]=='C') // right arrow key is pressed
   {
    if(!(rightstack.empty()))
     {
     leftstack.push(currentpath);
     string temp=rightstack.top();
     currentpath=temp;
     rightstack.pop();
     int i;
     for(i=0;i<currentpath.length();i++)
     { currpath[i]=currentpath[i];
     }
     currpath[i]='\0';
     currdirtracker=0;
     trackerforkeyl=0;
     tracker=1;
     printdirectory(currpath);
     resetcursor;
     }
   }
   else if(key[0]==':')
   {
    commandmode();
   }
  
  } 
    
} 


int main()
{ 
 EnableNormalMode();

 
 return 0;
}
