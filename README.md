# minishell

This project is all about recreating the shell, taking bash (Bourne Again Shell) as reference. This was a couple project, and I worked with [@lord-47](https://github.com/lord-47) :)

### What the Shell?

As we just said, we are asked to implement our own shell, but what is a shell to begin with? If we think of (for example) Linux as a nut or a seashell, the kernel/seed is the core of the nut and has to be surrounded by a cover or shell. Likewise, the shell we are implementing works as a command interpreter communicating with the OS kernel in a secure way, and allows us to perform a number tasks from a command line, namely execute commands, create or delete files or directories, or read and write content of files, among (many) other things

## Our Implementation of Minishell

The general idea for this shell is executing all the external shell commands and some of the internal commands. Before anything, it is highly recommended to take a deep dive into the [bash manual](https://www.gnu.org/software/bash/manual/bash.html), as it goes over every detail we had to have in mind when doing this project. The target of the project is working with pipes and processes.

### Shell External Commands

As mentioned before, the minishell can execute all system commands. You can also execute commands with pipes, so you can nest commands with a limit of up to 50 linked commands (which we think is more than enough). To connect commands you have to use the character '|'.

```shell
$ ls *.txt | grep test | wc -l
```

### Input and output redirection

You can redirect the input, output and errors of a command. To redirect the input you must use "< fileName", the output can be redirected with "> fileName" and the error using ">& fileName".

```shell
$ grep hello <input.txt >output.txt &>error.txt
```

### Execute Commands in Background

It also allows sending commands to the background. This allows you to continue using the minishell while a command is running. To send commands to the background you have to use the character '&'.

```shell
$ sleep 3 &
[1] 1726
```

### jobs Command

The jobs command displays the tasks that are running in the background.

```shell
$ jobs
[1]-  Stopped                 make -j4
[2]+  Stopped                 find . -name "*.java"
```

### fg Command

With the fg command you can bring to the foreground those tasks that are in the background. You can use it alone or with a number to indicate the command you want to bring from the background.

```shell
$ fg
$ fg 3
```

### cd Command

With cd you can change the directory you are in. If no path is passed to the command the working directory will be changed to the user's home directory.

```shell
$ cd ../
$ cd /home/user
$ cd
```

### umaks Command

With umask you can change the system mask. This allows you to change the permissions with which files are created. If you use umask without any arguments it will show the current system mask.

```shell
$ umask 0022
$ umask
0022
```

### exit Command

Exit closes the minishell and allows you to return to the normal shell.

### CTRL + C

Pressing ctrl+c will not close the minishell. If any command is running in the foreground and you press ctrl+c it will cancel the execution.

### Shell Prompt

El prompt de la shell es similar al de bash. Muestra username@hostName:workingDirectory$.

```shell
brais@brais-GL62M-7RDX:/home/brais$
```

## Installation

* Prerequisites

Make sure gcc compiler is installed:

* Cloning the Repositories

```shell
git clone github.com/BraisCabo/minishell
cd minishell
make
```

### Basic Usage

As per the norm, this project compiles an executable called ``minishell`` , and it is compiled using the ``.c`` files inside the folder

To execute the minishell you only have to give execution permissions

```shell
chmod u+x minishell
./minishell
```

After this the minishell should be running and you can use all its functionalities. If you want to close it execute the exit command.

## Summary

This was one of our biggest project yet, and it sure was challenging. Co-developing can be tricky but we had fun in the process and learn a lot about concurrent programming :)