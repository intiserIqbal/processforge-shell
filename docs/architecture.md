ProcessForge Shell Architecture

User Input
   |
   v
Parser (parser.c)
   |
   v
Executor (executor.c)
   |
   +--> Builtins (builtins.c)
   |
   +--> fork() -> execvp()
   |
   +--> waitpid()

Signals handled in signals.c