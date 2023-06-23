# Function fitter: simple but not too simple

## THIS IS WORK IN PROGRESS. IGNORE FOR NOW.

The aim of this project is to solve an interesting coding problem in multiple
programming environments, and then compare those environments on several
criteria:

- Ease and pleasure of coding

- Speed of execution

- Tooling including management of dependencies, "making", and packaging

The program implements a function optimisation algorithm. It takes an array of
doubles and a function as input and tries to minimize it. The reference
implementation is in Python and has the following characteristics, which all
the programming environments should attempt to have:

- It exposes an application programming interface that tries to return to its
  caller even if an error occurs.

- It works as a command line interface. It processes a complex set of command
  line arguments and prints out a helpful message on error.

- It makes extensive use of heap objects.

- It provides for parallel execution.

- It implements internationalization, or at least the coder's responsibility
  for this.

- It provides unit tests.

Also, the code in each programming environment must be reasonably iconic. I
suppose one could write a Basic program to be as fast as assembler if you poked
machine code directly into memory, but that would make it pointless to code in
Basic in the first place. So a rule I have stuck to is: don't abuse the
language to optimize.
