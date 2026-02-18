#ifndef SHELL_BUILTINS_FS_H
#define SHELL_BUILTINS_FS_H

void shell_builtins_fs_init(void);

int shell_builtin_ls(int argc, char **argv);
int shell_builtin_cat(int argc, char **argv);
int shell_builtin_pwd(int argc, char **argv);
int shell_builtin_cd(int argc, char **argv);
int shell_builtin_mkdir(int argc, char **argv);

#endif
