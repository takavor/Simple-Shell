# Simple-Shell

This is an implementation of a simple command-line shell. The shell supports bash commands as well as some built-in commands such as `fg` and `jobs`, executed either in foreground or background. The shell also features simple command piping and output redirection.

## Installation
```linux
git clone https://github.com/takavor/Simple-Shell.git
gcc simpleshell.c
./a.out
```
## Simple Demonstration
Here, I use the ping commands in the background by including `&` at the end of the command.

![image](https://user-images.githubusercontent.com/47959146/148695390-b5ae2bb8-d88a-420a-8b88-e041f8f7317c.png)

Here, I create and edit a file using vim, and demonstrate simple command piping.

![image](https://user-images.githubusercontent.com/47959146/148694828-dbbaafe6-a354-4e7c-a713-56d2a4aec7aa.png)

Here, I use output redirection to create a new file from the previous file.

![image](https://user-images.githubusercontent.com/47959146/148695450-0450f324-095f-4854-b516-edb19d8c75de.png)











