# Analysis of Fork and Exec Assignments

## Assignment 0: Multiple Fork Calls

The program creates a process hierarchy by executing three sequential `fork()` statements. Both the parent and each newly created child continue from the statement immediately following a successful fork.

Each `fork()` returns:

- **`-1`** if creation fails; no child is created.
- **`0`** in the child.
- **The child’s PID** in the parent.

The program checks each return value for `-1`. On failure, the affected process prints an error using `perror()` and returns `1`.

### Flow of execution

Initially, there is one process. The first fork creates one child. Both processes execute the second fork, creating two more children. The resulting four processes execute the third fork, creating four additional children.

| Fork statement | Number of processes executing it | New children created |
|---|---:|---:|
| First | 1 | 1 |
| Second | 2 | 2 |
| Third | 4 | 4 |

Assuming all calls succeed, the program creates **eight processes in total: the original process and seven descendants**. Although the source contains three fork statements, they execute seven times collectively.

Each process calls `getpid()` to obtain its own PID and `getppid()` to obtain its parent’s PID. It prints these values once and flushes standard output with `fflush(stdout)`.

### Process hierarchy

The observed PID and PPID values produce the following tree:

```text
414326 — shell
└── 415525 — original assignment process
    ├── 415526
    │   ├── 415528
    │   │   └── 415530
    │   └── 415529
    ├── 415527
    │   └── 415531
    └── 415532
```

The shell is outside the assignment’s count of eight processes.

The original process has three direct children. Four assignment processes are parents, and four have no children. A process can be both a child and a parent: for example, `415526` is a child of `415525` and the parent of `415528` and `415529`.

### Waiting and termination

Each process repeatedly calls:

```c
waitpid(-1, NULL, 0);
```

The `-1` selects any direct child, `NULL` means the program does not request the child’s exit status, and `0` allows the call to wait until a child terminates.

A successful call returns the collected child’s PID. On failure, it returns `-1`:

- `EINTR` means waiting was interrupted, so the program retries.
- `ECHILD` means there are no remaining children to collect, so the loop ends.
- Other errors cause the process to print an error and return `1`.

Waiting collects terminated children and keeps parents alive while their children finish. On the normal path, each process eventually returns `0`. If a fork fails, the immediate error return bypasses this waiting loop.

The printed lines can appear in different orders because the operating system schedules processes independently. A child may print before its parent. Therefore, the hierarchy is determined from PID–PPID relationships, not output order.

## Assignment 1: Simple Fork and Exec

The program creates one child process and uses that child to run `ls`. The parent waits for the child before printing its completion message.

### Flow of execution

The parent calls `fork()`. On success, the child receives `0`, while the parent receives the child’s PID. If creation fails, `fork()` returns `-1`, and the program reports the error and returns `1`.

The child enters the `pid == 0` branch and calls:

```c
execl("/bin/ls", "ls", (char *)NULL);
```

This replaces the child’s program with `ls`. The child keeps its PID and parent relationship. Because no directory argument is supplied, `ls` lists the current working directory inherited from the parent.

`execl()` is a library interface that performs program replacement through the underlying execution system call, `execve` on Linux. Successful execution does not return to the original program. On failure, `execl()` returns `-1`; the child prints an error and calls `_exit(127)`, which terminates it without returning.

### Process hierarchy

```text
Original parent
└── Child → runs ls → exits
```

There are **two processes total**. `execl()` replaces the child’s program; it does not create an additional process.

### Waiting and return values

The parent calls:

```c
waitpid(pid, &status, 0);
```

This waits for the specific child and stores its termination information in `status`. A successful call returns the child’s PID. A return value of `-1` indicates an error. The program retries on `EINTR`; other waiting errors cause it to return `1`.

After waiting, the parent prints:

```text
Parent process done
```

The `WIFEXITED(status)` macro checks whether the child exited normally. If so, `WEXITSTATUS(status)` extracts the child’s exit code, which the parent returns. If the child terminated abnormally, the parent returns `1`.

The observed directory listing appeared before the parent’s message. Waiting guarantees this order; `fork()` alone does not.

## Assignment 2: Multiple Forks and Execs

The program creates two children from the same parent. The first runs `ls`, and the second runs `date`. The parent waits after creating each child to guarantee the required output order.

### Flow of execution

The first `fork()` creates the first child. That child executes:

```c
execl("/bin/ls", "ls", (char *)NULL);
```

The parent calls `wait_for_child(first)` and waits for the first child to finish. This helper is a function defined in the program; it uses `waitpid()` and checks the resulting termination status.

After the first child finishes, the parent executes the second `fork()`. The second child calls:

```c
execl("/bin/date", "date", (char *)NULL);
```

The parent calls `wait_for_child(second)`. Once that child finishes, the parent prints its completion message.

The successful execution sequence is:

```text
Create first child
        ↓
ls prints directory listing and exits
        ↓
Parent collects first child
        ↓
Create second child
        ↓
date prints date and time and exits
        ↓
Parent collects second child
        ↓
Parent prints completion message
```

### Process hierarchy

```text
Original parent
├── First child → ls
└── Second child → date
```

There are **three processes over the successful run**, but at most two are alive simultaneously because the first child finishes before the second is created.

The children are siblings: they have the same parent. The first child never reaches the second fork because it either becomes `ls` or exits after an execution failure. Consequently, only the original parent creates the second child.

### Return values and synchronization

Each fork returns `0` in its child, a positive child PID in the parent, or `-1` on failure. Fork failure causes the parent to report the error and return `1`.

Each successful `execl()` does not return. On failure, it returns `-1`, and the child reports the error and terminates with `_exit(127)`.

The waiting helper retries `waitpid()` on `EINTR`. It returns the child’s exit code after normal termination, or `1` if waiting fails or the child terminates abnormally.

The parent stores both helper results. If either is nonzero, it returns `1`; otherwise, it returns `0`. The observed exit code was **`0`**, and the output order was:

```text
Directory listing
Date and time
Parent process done
```

Waiting for the first child before creating the second guarantees that `ls` finishes before `date` begins. Creating both children immediately and merely waiting for them in order would not guarantee their output order.

## Assignment 3: Fork and Exec with Arguments

The program creates one child and replaces its program with `echo`, passing a text message as an argument.

### Flow of execution

After a successful `fork()`, the child receives `0` and executes:

```c
execl("/bin/echo", "echo",
      "Hello from the child process", (char *)NULL);
```

The executable path is `/bin/echo`. The argument list supplied to the new program is:

| Argument | Value |
|---|---|
| `argv[0]` | `"echo"` |
| `argv[1]` | `"Hello from the child process"` |

The entire sentence is one argument because it is passed as one C string. The final `(char *)NULL` marks the end of the argument list.

The child becomes `echo`, prints the greeting, and exits. The parent waits, then prints its own message.

### Process hierarchy

```text
Original parent
└── Child → echo → prints greeting → exits
```

There are **two processes total**. Program replacement preserves the child’s PID and its relationship with the parent.

### Return values and observed behavior

`fork()` returns `-1` on failure, `0` in the child, and the child’s PID in the parent. The program checks for creation failure.

Successful `execl()` never returns. On failure, it returns `-1`, causing the child to print an error and terminate through `_exit(127)`.

The parent uses `waitpid()` to wait for the child, retrying if interrupted. After waiting successfully, it prints its message and returns the child’s exit code when normal termination is confirmed. Abnormal termination results in a return value of `1`.

The observed output was:

```text
Hello from the child process
Parent process done
```

The observed exit code was **`0`**. This confirms successful execution and the required ordering. The first line was printed by `echo`; the second was printed by the parent’s `printf()`.

## Assignment 4: Fork and Exec with Multiple Arguments

The program creates one child and uses `execl()` to run `grep` with a search pattern and a filename. The parent waits for the search to finish before printing its completion message.

### Flow of execution

After `fork()`, the child enters the branch selected by the return value `0` and executes:

```c
execl("/usr/bin/grep", "grep", "main", "test.txt",
      (char *)NULL);
```

The new program receives:

| Argument | Value | Purpose |
|---|---|---|
| `argv[0]` | `"grep"` | Program name |
| `argv[1]` | `"main"` | Search pattern |
| `argv[2]` | `"test.txt"` | Input filename |

The pattern and filename are separate arguments. `execl()` does not split a combined string into command-line arguments.

The child inherits the parent’s current working directory. Therefore, the relative filename `test.txt` refers to the file in `/home/vahagnt/fork_exec_homework`.

`grep` reads the file and prints complete matching lines. With the supplied arguments, `main` matches that sequence of characters, including within longer words.

### Process hierarchy

```text
Original parent
└── Child → grep → reads test.txt → prints matches → exits
```

There are **two processes total**. Reading the file does not create another process.

### Return values and synchronization

The program handles `fork()` and `execl()` errors as in the previous assignments. A failed fork returns `-1`. A failed `execl()` returns `-1`, after which the child reports the error and calls `_exit(127)`.

The parent calls `waitpid()` for the child and retries on `EINTR`. After waiting successfully, it prints:

```text
Parent process completed
```

It then checks the child’s termination status and returns the child’s normal exit code, or `1` for abnormal termination.

For `grep`, exit status has a specific meaning:

- **`0`** means at least one matching line was found.
- **`1`** means no matching lines were found.
- **`2`** is GNU `grep`’s normal error status.

A no-match result is different from failure to execute `grep`.

The observed output was:

```text
The main function starts the program.
The main topic is forks and child proccess and their PIDs.
Parent process completed
```

The observed exit code was **`0`**, indicating that matches were found successfully. Waiting ensured that the matching lines appeared before the parent’s message.
````
