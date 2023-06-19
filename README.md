# Function fitter: simple but not too simple

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

- It provides for multithreaded execution.

- It implements internationalization, or at least the coder's responsibility
  for this.
