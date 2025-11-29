#ifndef INTERNAL_H
#define INTERNAL_H

// Execute internal commands (cd, jobs)
// Returns: 0 = success, -1 = error
int execute_internal(char **args, int arg_count);

#endif // INTERNAL_H
