/* 
 * this program implements a command interpreter
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include "str.h"
#include "array.h"

//-------------------DATA TYPES AND ENUMS-----------------//

enum{
	LenLine = 100,
	MaxLines = 100,
	MaxCmds = 10, //Max number of commands at the pipe
	MaxArgv = 50,
	MaxPaths = 500,
	LenInt = 11 , // max len of an integer in a string
};

struct Cmd_a{
	int in;	//It will be -1 when in and out
	int out; //are stdin and stdout.
	int fd; /*fd that must be closed in son
			 *(when HERE = 1). -1 when fd is empty*/
	int argc;
	char *argv[MaxArgv];
	int wfme;
};
typedef struct Cmd_a Cmd_a;

struct Cmd_Pipe_a{
	Cmd_a cmd[MaxCmds];
	int ncmds;
};
typedef struct Cmd_Pipe_a Cmd_Pipe_a;

struct List_Fds{
	int fd[MaxCmds-1];
	int nelem;
};
typedef struct List_Fds List_Fds;

//-----------------------CODE----------------------------//

int is_pipe(char *line)
{
	return strchr(line, '|') != NULL;
}

int in_is_file(char *line)
{
	return strrchr(line, '<') != NULL;
}

int out_is_file(char *line)
{
	return strrchr(line, '>') != NULL;
}

void free_argv(Cmd_a *cmd)
{
	int i;
	
	for(i = 0; i < cmd->argc; i++){
		free(cmd->argv[i]);
	}
}

void free_cmdp(Cmd_Pipe_a *cmdp)
{
	int i;
	
	for(i = 0; i < cmdp->ncmds; i++){
		free_argv(&cmdp->cmd[i]);
	}
	free(cmdp);
}

Cmd_Pipe_a *new_cmdp(void)
{
	Cmd_Pipe_a *cmdp;
	int i;
	
	cmdp = malloc(sizeof(Cmd_Pipe_a));
	cmdp->ncmds = 0;
	for(i = 0; i < MaxCmds; i++){
		cmdp->cmd[i].fd = -1;
		cmdp->cmd[i].argc = 0;
	}
	return cmdp;
}

Cmd_a *new_cmd(void)
{
	Cmd_a *cmd;
	
	cmd = malloc(sizeof(Cmd_a));
	cmd->fd = -1;
	cmd->argc = 0;
	return cmd;
}

List_Fds *new_lfds(void)
{
	List_Fds *lfds;
	
	lfds = malloc(sizeof(List_Fds));
	lfds->nelem = 0;
	return lfds;
}

int backg(char *line)
{
	char *ptr;
	
	ptr = strrchr(line, '&');
	if(ptr == NULL){
		return 0;
	}else{
		return ptr + 1 == strchr(line, '\0');
	}
}

int is_assig(char *line)
{
	int i;
	int success;
	
	success = 0;
	for(i = 0; i<strlen(line); i++){
		if(line[i] == '='){
			success = 1;
			break;
		}else if(line[i] == ' ' || line[i] == '	'){
			break;
		}
	}
	return success;
}

void add_fd(int fd, List_Fds *lfds)
{
	lfds->fd[lfds->nelem] = fd;
	lfds->nelem++;
}

char *cmd_out(char *line)
{
	char *aux;
	
	if(!out_is_file(line)){
		return NULL;
	}
	line = strdup(line);
	aux = strrchr(line, '>');
	aux++;
	aux = rm_spaces(aux);
	aux = strsep(&aux, " <	");
	free(line);
	return aux;
}

char *cmd_in(char *line)
{
	char *aux;
	
	if(!in_is_file(line)){
		return NULL;
	}
	line = strdup(line);
	aux = strrchr(line, '<');
	aux++;
	aux = rm_spaces(aux);
	aux = strsep(&aux, " >	");
	free(line);
	return aux;
}

void rm_redir(char *ptr)
{
	for(;;){
		if(*ptr == '<' || *ptr == '>'){
			*ptr = '\0';
			break;
		}
		ptr++;
	}
}

void sv_argv(char *line, char *argv[], int *argc)
{
	char *p;
	char *token;
	
	line = strdup(line);
	token = strtok_r(line, " 	", &p);
	while (token != NULL) {
		argv[*argc] = strdup(token);
		*argc = *argc + 1;
		token = strtok_r(NULL, " 	", &p);
	}
	argv[*argc] = NULL;
	free(line);
}

int sv_out(Cmd_a *cmd, char *out, List_Fds *lfds)
{
	int fd;
	
	if(out == NULL){
		cmd->out = -1;
		return 0;
	}
	if((fd = open(out, O_CREAT|O_TRUNC|O_WRONLY, 
					   S_IRWXU|S_IRWXG|S_IRWXO)) == -1){
		fprintf(stderr, "can´t open file: %s\n", out);
		return 1;
	}
	cmd->out = fd;
	add_fd(fd, lfds);
	return 0;
}

int sv_in(Cmd_a *cmd, char *in, List_Fds *lfds)
{
	int fd;
	
	if(in == NULL){
		cmd->in = -1;
		return 0;
	}
	if((fd = open(in, O_RDONLY)) == -1){
		fprintf(stderr, "can´t open file: %s\n", in);
		return 1;
	}
	cmd->in = fd;
	add_fd(fd, lfds);
	return 0;
}

int sv_inout(Cmd_a *cmd, char *line, List_Fds *lfds)
{
	int success;
	char *in;
	char *out;
	
	in = cmd_in(line);
	out = cmd_out(line);
	success = sv_in(cmd, in, lfds) == 0 && sv_out(cmd, out, lfds) == 0;
	free(in);
	free(out);
	return !success;
}

int sv_cmd(char *line, Cmd_a *cmd, List_Fds *lfds)
{
	line = strdup(line);
	if(backg(line)){
		cmd->wfme = 0;
		sustlchr(line, '&', '\0');
	}else{
		cmd->wfme = 1;
	}
	if(sv_inout(cmd, line, lfds) != 0){
		return 1;
	}
	if(in_is_file(line) || out_is_file(line)){
		rm_redir(line);
	}
	sv_argv(line, cmd->argv, &cmd->argc);
	free(line);
	return 0;
}

void sv_pcmd(char *line, Cmd_a *cmd, int wfme)
{
	cmd->wfme = wfme;
	sv_argv(line, cmd->argv, &cmd->argc);
}

void sv_cmds(char *line, Cmd_Pipe_a *cmdp, int ncmds, int wfme)
{
	char *token;
	char *p;
	int i;
	
	line = strdup(line);
	if(in_is_file(line) || out_is_file(line)){
		rm_redir(line);
	}
	for(i = 0; i < ncmds; i++){
		if(i == 0){
			token = strtok_r(line, "|", &p);
		}else{
			token = strtok_r(NULL, "|", &p);
		}
		sv_pcmd(token, &cmdp->cmd[i], wfme);
	}
	free(line);
}

int cnct_cmds(char *line, Cmd_Pipe_a *cmdp, 
			  int ncmds, List_Fds *lfds)
{
	char *in;
	char *out;
	int i;
	int fd[2];
	
	line = strdup(line);
	in = cmd_in(line);
	out = cmd_out(line);
	if(sv_in(&cmdp->cmd[0], in, lfds) != 0 || 
			 sv_out(&cmdp->cmd[ncmds-1], out, lfds) != 0){
		return 1;
	}
	free(in);
	free(out);
	for(i = 0; i < ncmds-1; i++){
		pipe(fd);
		cmdp->cmd[i].out = fd[1];
		cmdp->cmd[i+1].in = fd[0];
		add_fd(fd[0], lfds);
		add_fd(fd[1], lfds);
	}
	free(line);
	return 0;
}

int sv_cmdp(char *line, Cmd_Pipe_a *cmdp, List_Fds *lfds)
{
	int wfme;
	int success;
	
	line = strdup(line);
	cmdp->ncmds = nchr(line, '|') + 1;
	if(backg(line)){
		wfme = 0;
		sustlchr(line, '&', '\0');
	}else{
		wfme = 1;
	}
	sv_cmds(line, cmdp, cmdp->ncmds, wfme);
	success = !cnct_cmds(line, cmdp, cmdp->ncmds, lfds);
	free(line);
	return !success;
}

/*Returns zero on success. Otherwise returns one.*/
int replace_env(char **line, char *str)
{
	char *ptr;
	char *value;
	
	ptr = str + 1;
	if ((value = getenv(ptr)) == NULL){
		fprintf(stderr, "error: var %s does not exist\n", ptr);
		return 1;
	}else{
		replace(line, str, value, LenLine);
		return 0;
	}
}

/*Returns zero on success. Otherwise returns one.*/
int replace_envs(char **line)
{
	char *token;
	char *p;
	char *aux;
	int success;
	
	success = 1;
	aux = strdup(*line);
	token = strtok_r(aux, " 	", &p);
	while (token != NULL && success) {
		if(token[0] == '$'){
			success = !replace_env(line, token);
		}
		token = strtok_r(NULL, " 	", &p); 
	}
	free(aux);
	return !success;
}

int run_verbose(Cmd_a *cmd, int *on)
{
	if(cmd->argc != 2){
		fprintf(stderr, "usage verbose on/off\n");
		return 1;
	}
	if(strcmp(cmd->argv[1], "on") == 0){
		*on = 1;
		return 0;
	}else if(strcmp(cmd->argv[1], "off") == 0){
		*on = 0;
		return 0;
	}else{
		fprintf(stderr, "usage verbose on/off\n");
		return 1;
	}
}

int run_cd(Cmd_a *cmd)
{
	if(cmd->argc == 1){
		chdir(getenv("HOME"));
		return 0;
	}else if(cmd->argc == 2){
		if(chdir(cmd->argv[1]) == -1){
			warn("cd: %s", cmd->argv[1]);
			return 1;
		}
		return 0;
	}else{
		fprintf(stderr, "cd: too many arguments\n");
		return 1;
	}
}

void add_env(char *line)
{
	char *name;
	char *value;
	
	line = strdup(line);
	value = line;
	name = strsep(&value, "=");
	setenv(name, value, 1);
	free(line);
}

char *create_paths(void)
{
	char *paths;
	char *str1;
	char *str2;
	int sz = MaxPaths; 
	
	str1 = getenv("PWD");
	str2 = getenv("PATH");
	paths = malloc(sz);
	snprintf(paths, sz, "%s:%s", str1, str2);
	return paths;
}

int is_on_path(char *name, char *path)
{
	DIR *d;
	struct dirent *ent;
	int found;
	
	found = 0;
	if ((d = opendir (path)) == NULL){
		return 0;
	}
	while ((ent = readdir(d)) != NULL){
		if (DT_REG == ent->d_type && 
			strcmp(ent->d_name, name) == 0){
			found = 1;
			break;
		}
	}
	closedir(d);
	return found;
}

char *cmd_path(char *name)
{
	char *paths;
	char *aux;
	char *path;
	char *token;
	char *p;
	
	path = NULL;
	paths = create_paths();
	aux = paths;

	token = strtok_r(paths, ":", &p);
	while (token != NULL) {
		if(is_on_path(name, token)){
			path = strdup(token);
		}
		token = strtok_r(NULL, ":", &p); 
	}
	free(aux);
	return path;
}

char *file_path(char *path, char *file)
{
	char *file_path;
  	int sz = strlen(path) + strlen(file) + 2;
	
	file_path = malloc(sz);
	snprintf(file_path, sz, "%s/%s", path, file);
	return file_path;
}

void change_out(int out)
{
	if(out != -1){
		dup2(out, 1);
		close(out);
	}
}

void change_in(int in, int wfme)
{
	int fd;
	
	if(in != -1){
		dup2(in, 0);
		close(in);
	}else{
		if(wfme == 0){
			fd = open("/dev/null", O_RDONLY);
			dup2(fd, 0);
			close(fd);
		}
	}
}

void change_fd(int in, int out, int wfme)
{
	change_in(in, wfme);
	change_out(out);
}

void close_fds(List_Fds *lfds)
{
	int i;
	
	for(i = 0; i < lfds->nelem; i++){
		close(lfds->fd[i]);
	}
	lfds->nelem = 0;
}

void run_cmd(Cmd_a *cmd, char *path, int *son_pid, List_Fds *lfds)
{
	int pid;
	char *filepath;
	
	filepath = file_path(path, cmd->argv[0]);
	pid = fork();
	if(pid == -1){
		err(1, "ERROR: err failed");
	}else if(pid == 0){
		change_fd(cmd->in, cmd->out, cmd->wfme);
		close_fds(lfds);
		if(cmd->fd != -1){
			close(cmd->fd);
		}
		execv(filepath, cmd->argv);
		errx(1, "ERROR: execl failed");
	}
	if(cmd->wfme == 1){
		*son_pid = pid;
	}
	free(filepath);
}

void exp_arg(char *argv[], int *argc, char *arg, int pos)
{
	glob_t globbuf;
	int error;
	
	if((error = glob(arg, 0, NULL, &globbuf)) != 0){
		if (error == GLOB_NOMATCH){
			return;
		}
		errx(1, "GLOB FAILED");
	}
	mv_slice(argv, *argc, pos+1, globbuf.gl_pathc-1, MaxArgv);
	ins_slice(argv, globbuf.gl_pathv, globbuf.gl_pathc, pos);
	*argc = *argc + globbuf.gl_pathc-1;
	globfree(&globbuf);
}

void exp_argv(char *argv[], int *argc)
{
	int i;
	
	i = 0;
	while(argv[i] != NULL){
		exp_arg(argv, argc, argv[i], i);
		i++;
	}
}

/*Returns 0 when the command is an 
 *executable file. Otherwise returns 1.*/
int proc_cmd(Cmd_a *cmd, int *pid, List_Fds *lfds, int *on)
{	
	char *path;
	char *name;
	
	name = cmd->argv[0];
	if(strcmp(name, "cd") == 0){
		return run_cd(cmd);
	}
	if(strcmp(name, "verbose") == 0){
		return run_verbose(cmd, on);
	}	
	if((path = cmd_path(name)) == NULL){
		fprintf(stderr, "%s: command not found\n", name);
		free(path);
		return 1;
	}
	exp_argv(cmd->argv, &cmd->argc);
	run_cmd(cmd, path, pid, lfds);
	free(path);
	return 0;
}

int all_cmds_exec(Cmd_Pipe_a *cmdp)
{
	int i;
	int success;
	char *path;
	char *name;
	
	success = 1;
	for(i = 0; i < cmdp->ncmds; i++){
		name = cmdp->cmd[i].argv[0];
		if(strcmp(name, "cd") == 0){
			fprintf(stderr, "builtin cd not allowed in command pipe\n");
			success = 0;
			break;
		}
		if((path = cmd_path(name)) == NULL){
			fprintf(stderr, "%s: command not found\n", name);
			success = 0;
			free(path);
			break;
		}else{
			free(path);
		}
	}
	return success;
}

int run_cmdp(Cmd_Pipe_a *cmdp, int *pid, List_Fds *lfds, int *on)
{
	int i;
	
	if(all_cmds_exec(cmdp)){
		for(i = 0; i < cmdp->ncmds; i++){
			proc_cmd(&cmdp->cmd[i], pid, lfds, on);
		}
		return 0;
	}
	*pid = -1;
	return 1;
}

/*Returns the writing side of the pipe*/
int cnct_dadcmd(Cmd_a *cmd, List_Fds *lfds)
{
	int fd[2];
	
	pipe(fd);
	cmd->in = fd[0];
	cmd->fd = fd[1];
	add_fd(fd[0], lfds);
	return fd[1];
}

/*Returns the writing side of the pipe*/
int cnct_dadcmdp(Cmd_Pipe_a *cmdp, List_Fds *lfds)
{
	int fd[2];
	int i;
	
	pipe(fd);
	cmdp->cmd[0].in = fd[0];
	for(i = 0; i < cmdp->ncmds; i++){
		cmdp->cmd[i].fd = fd[1];
	}
	add_fd(fd[0], lfds);
	return fd[1];
}

void write_son(int fd, char *data)
{
	if(write(fd, data, strlen(data)) != strlen(data)){
		errx(1, "Can't write in son\n");
	}
}

char *read_data(void)
{
	char *line;
	char *ptr;
	char *data;
	
	data = malloc(LenLine*MaxLines);
	line = malloc(LenLine);
	ptr = data;
	for(;;){
		fgets(line, LenLine, stdin);
		if(line[0] == '}' && line[1] == '\n'){
			break;
		}
		strcpy(ptr, line);
		ptr = strchr(data, '\0');
	}
	free(line);
	return data;
}

void do_cmdp(char *line, int *pid, int here, int *on)
{
	Cmd_Pipe_a *cmdp;
	int fd;
	char *data;
	List_Fds *lfds;
	
	cmdp = new_cmdp();
	lfds = new_lfds();
	if(sv_cmdp(line, cmdp, lfds) != 0){
		setenv("result", "1", 1);
		return;
	}
	if(here){
		fd = cnct_dadcmdp(cmdp, lfds);
		if(!run_cmdp(cmdp, pid, lfds, on)){
			close_fds(lfds);
			data = read_data();
			write_son(fd, data);
			free(data);
		}else{
			setenv("result", "1", 1);
			close_fds(lfds);
		}
		close(fd);
	}else{
		if(run_cmdp(cmdp, pid, lfds, on)){
			setenv("result", "1", 1);
		}
		close_fds(lfds);
	}
	free_cmdp(cmdp);
	free(lfds);
}

void do_cmd(char *line, int *pid, int here, int *on)
{
	Cmd_a *cmd;
	int fd;
	char *data;
	List_Fds *lfds;
	
	cmd = new_cmd();
	lfds = new_lfds();
	if(sv_cmd(line, cmd, lfds) != 0){
		setenv("result", "1", 1);
		return;
	}

	if(here){
		fd = cnct_dadcmd(cmd, lfds);
		if(!proc_cmd(cmd, pid, lfds, on)){
			close_fds(lfds);
			data = read_data();
			write_son(fd, data);
			free(data);
		}else{
			setenv("result", "1", 1);
		}
		close(fd);
	}else{
		if(proc_cmd(cmd, pid, lfds, on)){
			setenv("result", "1", 1);
		}
		close_fds(lfds);
	}
	free_argv(cmd);
	free(cmd);
	free(lfds);
}

/*Save the command PID in *pid. Save -1 when there is an
 *error or if the command has been executed in background.*/
void proc_line(char *line, int *pid, int *on)
{
	int here;

	if(is_fword(line, "ifok")){
		if(strcmp(getenv("result"), "0") != 0){
			return;
		}else{
			strsep(&line, " 	");
		}
	}
	if(is_fword(line, "ifnot")){
		if(strcmp(getenv("result"), "0") == 0){
			return;
		}else{
			strsep(&line, " 	");
		}
	}
	if(is_lword(line, "HERE{")){
		rm_lword(line);
		here = 1;
	}else{
		here = 0;
	}
	if(line[0] == '\0'){
		return;
	}
	if(is_assig(line)){
		add_env(line);
	}else if(is_pipe(line)){
		do_cmdp(line, pid, here, on);
	}else{
		do_cmd(line, pid, here, on);
	}
}

/*Wait for son. Returns the son's status*/
int wait_pid(int pid)
{
	int wsts;
	
	waitpid(pid, &wsts, 0);
	if (WIFEXITED(wsts)){	
		return WEXITSTATUS(wsts);
	}
	return 1;
}

/*Wait for children. Returns 0 when all children 
 * have ended successfully. otherwise returns 1.*/
int wait_children(void)
{
	int wsts;
	int children_sts = 0;
	
	while (wait(&wsts) != -1){
		if (!WIFEXITED(wsts) ||
			(WIFEXITED(wsts) && 
			WEXITSTATUS(wsts) != 0)) {
				
			children_sts = 1;
		}
	}
	return children_sts;
}

int
main(int argc, char *argv[])
{
	char *line;
	int pid = -1;
	char *result;
	int sts;
	int on = 0;
	
	setenv("result", "0", 1);
	line = malloc(LenLine);
	for(;;){
		if(fgets(line, LenLine, stdin) == NULL){
			printf("\n");
			break;
		}
		if(line[0] == '\n'){
			continue;
		}
		
		sustchr(line, '\n', '\0');
		if(replace_envs(&line) == 0){
			proc_line(line, &pid, &on);
			if(on){
				if(write(2, line, strlen(line)) != strlen(line)){
					errx(1, "ERROR: can't write in stderr");
				}
				fprintf(stderr, "\n");
			}
		}
		if(pid != -1){
			result = itoc(wait_pid(pid), LenInt);
			setenv("result", result, 1);
			free(result);
		}
	}
	free(line);
	sts = wait_children();
	exit(sts);
}
