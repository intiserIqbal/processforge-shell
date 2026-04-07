Demo plan:

1. Build the shell:
   - `make clean && make`

2. Run the shell:
   - `./build/processforge`

3. Exercise built-ins:
   - `help`
   - `cd <directory>`
   - `exit`

4. Run external commands:
   - `ls`
   - `pwd`
   - `cat include/shell.h`

5. Demonstrate redirection:
   - `echo hello > out.txt`
   - `cat < out.txt`
   - `echo world >> out.txt`

6. Demonstrate pipes:
   - `ls | grep txt`
   - `cat include/shell.h | grep typedef`

7. Demonstrate background execution:
   - `sleep 5 &`
   - confirm the shell prompt returns immediately

8. Validate with tests:
   - `./tests/test_builtins.sh`
   - `./tests/test_redirection.sh`
   - `./tests/test_pipes.sh`

9. Review source mapping:
   - `src/main.c` for the prompt loop and SIGINT handling
   - `src/parser.c` for parsing tokens, redirection, and pipes
   - `src/executor.c` for fork/exec, redirection, and pipeline execution
   - `src/builtins.c` for built-in command behavior

Notes:
- The prompt is produced by `src/main.c`.
- Redirection and pipes are implemented in `src/executor.c`.
- There is no separate `signals.c`; signal handling is set up in `src/main.c`.
- `tests/` contains shell integration tests, fixtures, and expected output files.