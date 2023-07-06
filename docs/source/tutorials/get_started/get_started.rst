Get Started
===========

.. todo::
    * First step, cloning and demo layer (how to build it)
    * Defining our environment (simple, one UUT with a few test points)
    * Defining our first sequences (simple, have a few requires)

This document is intended to help you set up the software development environment for Frasy and its ecosystem.
After that, a simple example will show you how to define your own environment, as well as writing your own sequences of tests.


Introduction
------------

**Frasy** is a tool allowing technicians and engineers to test any Printed Circuit Board Assemblies (PCBA) in a highly scalable manner.

Thanks to its usage of a powerful and versatile, yet simple scripting language (Lua), the learning period is very short, and possibilities are endless, allowing for a very fast iterative approach towards test development.

What You Need
-------------

Hardware
^^^^^^^^

* A computer supporting opengl 4.5 and more recent.
* :ref:`custom_hardware` compatible with Frasy.

.. note::
    Currently, only Windows machines are officially supported.
    Linux support is non-officially and may not completely work.

Software
^^^^^^^^

One of the following:

* A pre-built version of Frasy
* A local version of Frasy, built from `the source <https://github.com/smartel99/Frasy.git>`_
* A custom version of Frasy (See :ref:`custom_layer`).

.. todo::
    * Implement CI/CD on Frasy to build the demo version.
