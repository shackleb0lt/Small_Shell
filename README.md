# Small Shell

My implementation of linux shell in C, refer to the guide for detailed description of functions and usage.

* Handles variable assignment and expansion
* Piping of commands  ( ```|``` )
* Only Output redirection ( ```>``` and ```>>``` ) is supported for now.
* Catches SIGINT and prevents interruption.
* Inbuild commands are 
    * *cd* &nbsp; Change Current Directory
    * *showvar* &nbsp; Print shell variables
    * *showenv* &nbsp; Print environment varaibles
    * *export* &nbsp; Export variable to environment scope
    * *unset* &nbsp; Remove varaible from environment scope
    * *pushd* &nbsp; Add directory node to top od directory stack
    * *popd* &nbsp; Remove directory node from stack
    * *dirs* &nbsp; Print directory stack
    * *source* &nbsp; Execute commands from script file
    * *exit* &nbsp; Exit the shell

To compile you need the readline package , run the following command in terminal to install the package, ignore if already installed.
```bash
sudo apt-get install libreadline8 libreadline-dev
```
The readline library provides line auto-completion and suggestion with history functionality.

To compile using gcc and execute run the following command.
```bash
make all; ./shell 
```
Exit the shell by typing *exit* or *quit* .