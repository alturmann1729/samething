# samething

samething is a Specific Area Message Encoding (SAME) header generator.

**WARNING**: The author is not responsible for the misuse of this software. The
output of this software has the potential to activate EAS decoder systems in the
continental United States of America and its territories, along with various
offshore marine areas, if transmitted.

Do **not** transmit these tones over any radio communication unless so
authorized.

Do **not** play these tones in public areas, regardless of your jurisdiction.

If you do not heed the above warnings, particularly if you are under U.S.
jurisdiction, [see what the FCC will do to you](https://www.fcc.gov/enforcement/orders/1830),
not counting the possible criminal penalties you will face if you did so
maliciously.

This was written for an educational purpose, not a malicious one.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Introduction

samething allows a user to generate Specific Area Message Encoding (SAME) header
tones. It complies with the latest revision of the [EAS protocol](https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-11/subpart-B/section-11.31),
updated 4/03/2023.

A native user interface is provided for Windows, macOS and Linux users to allow
ease and simplicity of generation. It also serves to demonstrate how to
integrate the core into a frontend.

The core does not make use dynamic memory allocation, recursion, dynamic
dispatch, RTTI, exceptions, or the C++ Standard Template Library (STL). These
choices were made for efficiency reasons and to facilitate ports to resource
constrained systems.

## Building the project

This project uses highly specialized toolchains, essentially a self-contained
bundle of tools and libraries specifically geared for use in building this
project in its entirety. Attempting to build the project without using a
sanctioned toolchain package is entirely unsupported, and I wish you the best of
luck if you are going that route.
