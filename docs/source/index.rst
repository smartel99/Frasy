.. Frasy documentation master file, created by
   sphinx-quickstart on Thu Jun 15 15:19:27 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to Frasy's documentation!
=================================

.. image:: assets/img/frasy_logo.svg

**Frasy** is a tool allowing technicians to test any Printed Circuit Board Assemblies (PCBA) in a highly scalable manner, ranging from small (less than 1000 units per year) to very large (> 10 million units per year) production lines.

Using a descriptive approach, very little programming knowledge are required in order to write test sequences. The technician only has to specify the criteria for a PCBA to be valid, and Frasy handles the rest!

.. note::
   This project is under active development.

Getting Started
===============

To generate the project files, use one of the generation scripts found in ``scripts/Windows``.
Alternatively, invoke Premake directly.

.. note::
   A compiler supporting C++20 is required to build Frasy!

By default, Frasy builds the demonstration mode. The sources for this demo mode can be found in the ``src/demo_mode`` directory.

To provide your own source code, use the ``--src_loc`` flag when generating the project files.

.. todo::
   Is that really how it's done?

Thanks
======
Thank you to the following persons for their help in making this project possible!

* `DoubleNom <https://gitlab.com/DoubleNom>`_ for the help designing and implementing the entire architecture and for his helpful criticism :)
* `nickclark2016 <https://github.com/nickclark2016>`_ for designing the test description procedure!
* An anonymous friend for the help coming up with sentences that actually makes sense!

Contents
========

.. toctree::
   :maxdepth: 3
   :glob:
   
   tutorials/tutorials
   sdk/sdk
   todo
   