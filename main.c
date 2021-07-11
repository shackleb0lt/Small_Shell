/* Inlcuding Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Struct definitions */
/* Struct to store variables as key-value pair */
typedef struct var{
    char *key;
    char *value;
    struct var* next;
} var;

/* Struct to store shell varaibles */
typedef struct shell_var
{
    char *PROMPT;
    char *PATH;
    char *CWD;
    char *USER;
    char *HOME;
    char *SHELL;
    char *TERMINAL;

	//Linked list of new variables created by user
	var * head;
	var * tail;

} shell_var;

/* Struct to Store directory stack as linked list */ 
typedef struct dir_node
{
	char * path;
	struct dir_node * next;
} dir_node;

/* Initializing Global Variables*/
dir_node * dir_stack = NULL;		// Head of directory stack
shell_var main_shell_var;			// Global instance of shell_var struct
extern char **environ;				// Array of string which stores environment variable by default
char *cmds[32];						// Array of strings storing commands separated by pipes
char *argV[32];						// Array of string storing argument for commands
char *op_file;						// Name of output redirection file
char *line;							// To store the input from readline()
char *ret;							// Pointer to store return value of stchr()
int background_flag;				// Flag to store whether the process runs in background
int exit_flag;						
int pid;							// Process Id from fork()

/* Declaring Functions */
void show_env_variables(char *);							// Prints the environment variables
void export_var(char *);									// Promotes the shell variable to environment
void unset_var(char *);										// Removes the environment Variable

void set_shell_variables();									// Initializes the standard shell variables
void set_prompt();											// Updates the PROMPT.
void show_shell_variables(char *);							// Prints the shell variables
char * variable_get_value(char *);							// Returns the value of variable given it's key
void variable_set_value(char *,char *);						// Creates a new variable entry

void change_directory(char *);								// Change the current working directory
void print_dir_stack();										// Prints the directory stack
void pop_dir_stack();										// Pops the top of directory stack and changes current working directory
void push_dir_stack(char *);								// Pushes a valid directory path onto directory stack and changes current working directory

static int execute_cmd(char *, int , int , int );			// Exectues inbuild and external commands handles Output Redirection
void parse_expand_var();									// Parses the input line from user to check for variable assignment/expansion
void remove_qoute(char *);									// Removes qoutes for echo command
int op_redirect(char *);									// Returns a File descriptor for output redirection > and >>
void parse();												// Splits the ouput after parse_expand_var() into commands separated by pipes

/* Function to handle SIGINT i.e Ctrl-C interrupts from user */
void sigintHandler(int sig_num) {
    // signal(SIGINT, sigintHandler);
    fflush(stdout);
    return;
}

/**
 * Prints all the environment variables if key is NULL 
 * @param key prints the value of environment variable stored in key
 */
void show_env_variables(char *key)
{
	if ((key==NULL) || (!strcmp(key, "\n")) || (!strcmp(key,"\0")) )
	{
		char **s = environ;
		for (; *s; s++) {
			printf("%s\n", *s);
		}
		return;
	}
	char * value = getenv(key);	
	if(value != NULL)
	{
		printf("%s=%s\n",key,value);
		return;
	}
	perror("Environment Variable does not exist");
	return;
}

/**
 * Promotes the shell variable to environment
 * @param key finds if variable stored in key exists and promotes them to environment scope
 */
void export_var(char * key)
{
	if ((key==NULL) || (!strcmp(key, "\n")) || (!strcmp(key,"\0")) )
	{
		perror("Usage: export <variable name>");
		return;
	}
	char * value = variable_get_value(key);
	if(value==NULL)
	{
		perror("Variable not found");
		return;
	}
	setenv(key,value,1);
	return;
}

/**
 * Removes the environment Variable stored in key
 * @param key
*/
void unset_var(char *key)
{
	if ((key==NULL) || (!strcmp(key, "\n")) || (!strcmp(key,"\0")) )
	{
		perror("Usage: unset <variable name>");
		return;
	}
	unsetenv(key);
	return;
}

/* Initializes the standard shell variables */
void set_shell_variables()
{
    main_shell_var.PATH = strdup(getenv("PATH"));
    main_shell_var.SHELL = strdup(getenv("SHELL"));
    main_shell_var.USER = strdup(getenv("USERNAME"));
    main_shell_var.HOME = strdup(getenv("HOME"));
    main_shell_var.CWD = strdup(getenv("PWD"));
    main_shell_var.TERMINAL = strdup(ctermid(NULL));

    main_shell_var.PROMPT = (char *)malloc(sizeof(char)*256);
	set_prompt();
   
	main_shell_var.head = NULL;

	dir_stack = (dir_node*)malloc(sizeof(dir_node));
	dir_stack->next = NULL;
	dir_stack->path = strdup(main_shell_var.CWD);	
}

/* Updates the PROMPT */
void set_prompt()
{
	strcpy(main_shell_var.PROMPT,"\033[0;32m");
	strcat(main_shell_var.PROMPT,main_shell_var.USER);
	strcat(main_shell_var.PROMPT,"\033[0m");
    strcat(main_shell_var.PROMPT,":");
	strcat(main_shell_var.PROMPT,"\033[0;36m");
    strcat(main_shell_var.PROMPT,main_shell_var.CWD);
	strcat(main_shell_var.PROMPT,"\033[0m");
    strcat(main_shell_var.PROMPT,"$ ");
}

/**
 * Prints all the shell variables if key is NULL
 * @param key prints the value of shell variable stored in key
*/
void show_shell_variables(char *key)
{
	if ((key==NULL) || (!strcmp(key,"\0")))
	{
		printf("PATH=%s\n",main_shell_var.PATH);
		printf("PROMPT=%s\n",main_shell_var.PROMPT);
		printf("SHELL=%s\n",main_shell_var.SHELL);
		printf("USER=%s\n",main_shell_var.USER);
		printf("HOME=%s\n",main_shell_var.HOME);
		printf("CWD=%s\n",main_shell_var.CWD);
		printf("TERMINAL=%s\n",main_shell_var.TERMINAL);

		var * temp = main_shell_var.head;
		while(temp!=NULL)
		{
			printf("%s=%s\n",temp->key,temp->value);
			temp = temp->next;
		}
		return;
	}
	
	char * value = variable_get_value(key);
	if(value!=NULL)
	{
		printf("%s=%s\n",key,value);
		return;
	}
	perror("Variable Not Found");
}

/**
 * Finds and Returns the value of variable given it's key 
 * @param key finds the value of variable name stored in key
 * @return variable value or NULL if key is invalid
 * */
char * variable_get_value(char * key)
{
	if(!strcmp(key,"PROMPT")) return main_shell_var.PROMPT;
    else if(!strcmp(key,"PATH")) return main_shell_var.PATH;
    else if(!strcmp(key,"CWD")) return main_shell_var.CWD;
    else if(!strcmp(key,"SHELL")) return main_shell_var.SHELL;
    else if(!strcmp(key,"USER")) return main_shell_var.USER;
    else if(!strcmp(key,"HOME")) return main_shell_var.HOME;
    else if(!strcmp(key,"TERMINAL")) return main_shell_var.TERMINAL;
	
	var * temp = main_shell_var.head;
	while(temp!=NULL)
	{
		if(!strcmp(temp->key,key)){
			return temp->value;
		}
		temp = temp->next;
	}
	return NULL;
}

/**
 * Creates a new variable entry int he linked list
 * @param key stores the name of variable
 * @param value stores the value of variable
 */
void variable_set_value(char* key,char* value)
{
    if(strcmp(key,"PATH")==0)
    {
        free(main_shell_var.PATH);
        main_shell_var.PATH = strdup(value);
		return;
    }
    else if(strcmp(key,"CWD")==0){
        free(main_shell_var.CWD);
        main_shell_var.CWD = strdup(value);
		return;
    }
    else if(strcmp(key,"SHELL")==0){
        free(main_shell_var.SHELL);
        main_shell_var.SHELL = strdup(value);
		return;
    }
    else if(strcmp(key,"USER")==0){
        free(main_shell_var.USER);
        main_shell_var.USER = strdup(value);
		return;
    }
    else if(strcmp(key,"HOME")==0){
        free(main_shell_var.HOME);
        main_shell_var.HOME = strdup(value);
		return;
    }
    else if(strcmp(key,"TERMINAL")==0){
        free(main_shell_var.TERMINAL);
        main_shell_var.TERMINAL = strdup(value);
		return;
    }
	var *temp = main_shell_var.head;
	while(temp!=NULL)
	{
		if(strcmp(temp->key,key)==0)
        {
            free(temp->value);
			temp->value = strdup(value);
            return;
        }
		temp=temp->next;
	}

	temp = (var*)malloc(sizeof(var));
	temp->key = strdup(key);
	temp->value = strdup(value);
	temp->next = NULL;

	if(main_shell_var.head==NULL)
	{
		main_shell_var.head = temp;
		main_shell_var.tail = temp;
		return;
	}
	main_shell_var.tail->next = temp;
	main_shell_var.tail = main_shell_var.tail->next;
	return;	
}

/**
 * Change the current working directory
 * @param path stores the new working directory path
*/
void change_directory(char *path)
{
    if(path==NULL || (!strcmp(path,"~")) || (!strcmp(path,"~/")) )
    {
        if(chdir(main_shell_var.HOME)<0)printf("bala\n");
    }
    else if(chdir(path) < 0){
		perror("No such file or directory: ");
		return;
	}
    char path_buf[256];

    if (getcwd(path_buf, sizeof(path_buf)) != NULL) {
		main_shell_var.CWD = strdup(path_buf);
	}
	set_prompt();
	
}

/**
 * Prints the directory Stack 
 */
void print_dir_stack()
{
	dir_node * temp = dir_stack;
	while(temp!=NULL)
	{
		printf("%s\t",temp->path);
		temp=temp->next;
	}
	printf("\n");
}

/**
 * Pops the top of directory stack and changes current working directory
 * If only one node is present in the directory stack then does nothing
 */
void pop_dir_stack()
{
	dir_node *temp = dir_stack;
	if(temp->next == NULL)
	{
		perror("popd: directory stack empty");
		return;
	}
	dir_stack = dir_stack->next;
	change_directory(dir_stack->path);
	free(temp);
	return;
}

/**
 * Pushes a valid directory path onto directory stack
 * Also changes current working directory to the path
 * @param path stores the path to new directory node
*/
void push_dir_stack(char * path)
{
	if ((path==NULL) || (!strcmp(path, "\n")) || (!strcmp(path,"\0")) )
	{
		perror("Usage: pushd pathname");
		return;
	}
    change_directory(path);

	dir_node * temp = (dir_node *) malloc(sizeof(dir_node));
	temp->path = strdup(main_shell_var.CWD);
	temp->next = dir_stack;
	dir_stack = temp;

}

/**
 * Parses the input line from user to check for variable assignment/expansion
 * If Variable assignment are present then adds creates or updates variables in linked list
 * In case of variable expansion rewrites the variable value inplace of key in new buffer
 */
void parse_expand_var()
{
	int ignore = 0,i = 0,j = 0;
	int last_space = -1;
	char buf[1024];
	char key[64];
	
	// Variable assignment operation
	while (line[i]!='\0')
	{
		if(line[i]==' ') last_space = i;
		if(line[i]=='"') ignore = !ignore;
		if(line[i]=='=' && !ignore)
		{
			if(i==0 || line[i-1]==' ' || line[i+1]==' ' ) {
				perror("Variable Assignment Error");
				return;
			}

			int k = 0;
			int m = last_space + 1;
            if(last_space==-1) m=0;
        
			while(line[m]!=' ' && line[m]!='\0' && line[m]!='\n') buf[k++]=line[m++];
            buf[k]='\0';

			for(k=last_space+1;k<m;k++) line[k]=' ';
            char * mykey = strtok(buf,"=");
            char * value = strtok(NULL,"= ");

			if(mykey!=NULL && value!=NULL){
				variable_set_value(mykey,value);
			}	
		}
		i++;	
	}

	i=0;j=0;
    buf[0]='\0';

	// Variable expnasion 
	while (line[i]!='\0')
	{
		if( (i==0 &&line[i]=='$') || (i!=0 && line[i-1]!='\\' && line[i]=='$' ))
		{
			i++;
			int k=0;

            if(line[i]=='{')
            {
                i++;
                while(line[i]!='}' && line[i]!='\0'  && line[i]!='\n')key[k++]=line[i++];
                i++;
            }
            else
            {
                while(line[i]!=' ' && line[i]!=34 && line[i]!=39 && line[i]!='\0' && line[i]!='\n')key[k++]=line[i++];
            }
			
			key[k]='\0';
			char * token = strtok(key," {}");
			char * value = variable_get_value(token);
			if(value==NULL)value = " ";

			for(k=0;k<strlen(value);k++)buf[j++]=value[k];
		}
		
		buf[j++]=line[i++];
	}

	buf[j]='\0';
    free(line);
	line = strdup(buf);
}

/**
 * Removes non escaped double/single qoutes for echo command
 */
void remove_qoute(char * str)
{
	for(int i=1;i<strlen(str);i++)
		if(str[i-1]!='\\' && (str[i]==34 || str[i]==39) )str[i]=' ';
}

/**
 * Extracts file name of output redirection file
 * Opens the file in Read or Append mode if > or >> respectively
 * @param cmd command containg the Output redirection.
 * @return file descriptor of the open file to redirect output stream
 */
int op_redirect(char * cmd)
{
	ret = strchr(cmd,'>');
	(*ret)='\0';
	int op;
	if(*(ret+1)=='>')
	{
		op_file = strtok(ret+2,"> ");
		op=open(op_file,O_CREAT | O_RDWR | O_APPEND,0666);
		
	}
	else
	{
		op_file = strtok(ret+1,"> ");
		op=open(op_file,O_CREAT | O_RDWR | O_TRUNC,0666);
	}

	return op;
}

/**
 * Function that executes inbuild or external commands tokenized by pipes
 * @param cmd command name
 * @param inputfd file descriptor if input is to be taken from instead of stdin
 * @param first whether the command is first 
 * @param last whether the command is last
 * @return file descriptor for the read end of pipe for the input of next piped command
*/
static int execute_cmd(char * cmd, int inputfd, int first, int last)
{
	if((ret = strchr(cmd,'&'))!=NULL)
	{
		background_flag = 1;
		*ret=' ';
	}
	if(strstr(cmd,"echo")!=NULL)
	{
		if(strchr(cmd,34)!=NULL || strchr(cmd,39)!=NULL)
			remove_qoute(cmd);
	}
    int op_fd=-1;
	if((ret = strchr(cmd,'>'))!=NULL)
	{
		op_fd = op_redirect(cmd);
	}
	int index=0;
    argV[index++] = strtok(cmd, " ");
    while ((argV[index] = strtok(NULL, " "))!= NULL ) index++;
    argV[index]=NULL;

    
	int pipefd[2];

    if (pipe(pipefd) < 0) {
		perror("Pipe error: ");
		return 1;
	}
    pid = fork();

    if(pid == 0)
    {
        if(first==1 && last==0 && inputfd == 0)
        {
            dup2(pipefd[1],1);
        }
        else if(first==0 && last==0 && inputfd != 0)
        {
            dup2(inputfd,0);
            dup2(pipefd[1],1);
        }
        else if(first == 0 && last==1)
        {
            dup2(inputfd,0);
        }
		
		if(op_fd>=0)
		{
			dup2(op_fd, 1);
			close(op_fd);
		}
        if(argV[0]==NULL) exit(0);
        else if (!strcmp(argV[0],"cd") || !strcmp(argV[0], "export") || !strcmp(argV[0], "unset") || !strcmp(argV[0], "popd") || !strcmp(argV[0], "pushd")) {
			exit(0);
		}
        else if (!strcmp(argV[0], "showvar")) {
			show_shell_variables(argV[1]);
            exit(0);
		}
        else if (!strcmp(argV[0], "showenv")) {		
			show_env_variables(argV[1]);
            exit(0);
		}
        else if (!strcmp(argV[0], "dirs")) {		
			print_dir_stack();
            exit(0);
		}
		else if (!strcmp(argV[0], "whoami") && argV[1]==NULL) {
			printf("%s\n",main_shell_var.USER);
            exit(0);
		}
		else if (!strcmp(argV[0], "pwd")) {
			printf("%s\n",main_shell_var.CWD);
            exit(0);
		}
		else if(!strcmp(argV[0], "source"))
		{
			if(argV[1]==NULL)
			{
				printf("Usage: source <filename>\n");
				exit(0);
			}
			argV[0]=strdup("bash");
		}
        if (execvp(argV[0], argV) < 0) {
			fprintf(stderr, "%s: Command not found\n",argV[0]);
		}
        exit(0);
    }

	if(pid !=0 )
	{
		waitpid(pid,0,0);

		if(argV[0]==NULL)
		{
			if (inputfd!=0)
				close(inputfd);
			
			if (last==1)
        		close(pipefd[0]);

			close(pipefd[1]);
			return pipefd[0];;
		}
        if (!strcmp(argV[0],"cd")) {
			change_directory(argV[1]);   
		}
		else if (!strcmp(argV[0], "popd")) {	
			pop_dir_stack();
		}
		else if (!strcmp(argV[0], "pushd")) {	
			push_dir_stack(argV[1]);
		}
		else if (!strcmp(argV[0], "export")) {
			export_var(argV[1]);    
		}
        else if (!strcmp(argV[0], "unset")) {
			unset_var(argV[1]);
		}
	}

    if (last==1)
        close(pipefd[0]);

    if (inputfd != 0)
        close(inputfd);

    close(pipefd[1]);

    return pipefd[0];
}

/**
 * Parse and tokenize the command by pipes
 * Pass appropriate file descriptos for I/O
*/
void parse()
{
    int index=0;
    cmds[index++] = strtok(line, "|");
    while ((cmds[index] = strtok(NULL, "|"))!= NULL ) index++;
    cmds[index]=NULL;
    
    int input_fd = 0;
    int first=1;

    for(int i=0; i<index-1 ;i++)
    {
        input_fd = execute_cmd(cmds[i],input_fd,first,0);
        first = 0;
    }
    execute_cmd(cmds[index-1],input_fd,first,1);
}

//Main Function handles infinite REPL until exit command is provided
int main(int argc, char* argv[])
{
    int status;
    signal(SIGINT, sigintHandler);
    set_shell_variables();
    using_history();
  
    printf("\033[1;34m\n *** Welcome to Shell ***\n\033[0m");
    printf("\n");

    do
    {
		background_flag = 0;
        line = readline(main_shell_var.PROMPT);
        if (!(strcmp(line, "\n") && strcmp(line,"")))
			continue;

        add_history (line);

        if (!(strncmp(line, "exit", 4) && strncmp(line, "quit", 4))) {
			exit_flag = 1;
			break;
		}

        parse_expand_var();
        parse();

		waitpid(pid,&status,0);
		status = 0;

        free(line);

    } while(!WIFEXITED(status) || !WIFSIGNALED(status));

    if (exit_flag == 1) {
		printf("\033[1;34m\n *** Thank You! ***\n\033[0m");
    	printf("\n");
		exit(0);
	}

    return 0;
}