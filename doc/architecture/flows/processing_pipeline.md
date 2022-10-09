<!-- omit in toc -->
# Processing Done During Sequence Execution

- [Loading](#loading)
  - [Fetching](#fetching)
  - [Spawning](#spawning)
- [Validation](#validation)
- [Optimizing](#optimizing)
- [Executing](#executing)
- [Archiving](#archiving)

# Loading

## Fetching
Loads every sequence found in the loaded files. If a filter on sequences is specified, only sequences matching that filter are loaded.

## Spawning
An instance of the sequence is spawned for each UUT that needs to be tested, each individual instances receiving the mapping table associated to that UUT, 
as well as the sequence's context (serial number, version information, operator, date, etc.)

Each instance then loads the tests that needs to be done, building one big list of tests to do.

# Validation
The tests are validated to ensure that the resources requested are available, i.e. that there are no tests asking for features that the hardware is unable to do.

Firstly, a list of the resources is required by executing a dry run of the tests. 
This allows the validator to know which sub-systems are needed, as well as to determine the peak utilization of each sub-system.

# Optimizing
For now, this layer only forwards the task list to the executor.
A very minimal form of optimization will be done at first, where identical tests (i.e. tests that are done on every UUTs) will be executed in parallel.

In the future, the optimizer will be charged to:
- Break each test into the order lists 
- Analyze the impact that each test have on the UUT(s) based on the mapping and sub-division of the panel.
  - What part of the circuit do they affect
  - What resources are used
- Group the tests that can be together


# Executing

# Archiving