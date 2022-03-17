void eval(char* cmdline)
{
    char* argv[MAXARGS];  /* argument list execve() */
    pid_t pid;
    int bg;  /* should the job runs in bg or fg */
    sigset_t mask;

    sigemptyset(&mask);

    bg = parseline(cmdline, argv);
    if (argv[0] == NULL) { /* ignore empty lines */
        return;
    }

    if (!builtin_cmd(argv)) {  /* no need to fork buildin command */
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);  /* block SIGCHLD */
        if ((pid = fork()) == 0) {  /* child runs the job */
            sigprocmask(SIG_UNBLOCK, &mask, NULL); /* unblock SIGCHLD in child */
            setpgid(0, 0);  /* puts the child in a new process group, GID = PID */
            if (execve(argv[0], argv, environ) < 0) {
                fprintf(stderr, "%s: Command not found\n", argv[0]);
                _exit(1);
            }
        }
        /* adds the child to job list */
        addjob(jobs, pid, (bg ? BG : FG), cmdline);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);  /* unblock SIGCHLD */

        if (!bg) {  /* parent waits for fg job terminate */
            waitfg(pid);
        }
        else {  /* shows information of bg job */
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
    }
    return;
}