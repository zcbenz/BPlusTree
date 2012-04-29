BPlusTree
==========

This is my simple implementation of B+ Tree, the keys, values, and nodes are of
fixed size, and you need to recompile when changing some important parameters.

The main advantages of my implementation is:

 * Store data in file, can manipulate huge data
 * Code is very small, indeed smaller than even most in-memory implementations
 * Has good test code (longer than implementation itself)

Feel free to use my code, I would be happy if it helps you :-)

Compile
-------

Under linux, compile by using:

    make

And do the unit test:

    make test

If you use IDE or work on Windows, just drag all files into a project and
click the compile button.

Files
-----

`bpt.h` and `bpt.cc` is the implementation of B+ tree. `predefined.h` 
defines the tree order, key/value type, key compare function and other
tree settings, modify it to satify your need. Just include these tree files 
in your project to use the B+ tree.

There are some demonstration tools under `util` folder:

`unit_test.cc` and `unit_test_predefined.h` is the unit test code.

`dump_numbers.cc` can write some numbers into a database, so you can quickly
test out the B+ tree.

`cli.cc` is a command tool to manipulate an exisiting database.

By default, the key type is 16 byte string and value type is int. the
`keycmp` function is written to easily compare number strings.

Examples
--------

Create a database and insert numbers from `0` to `100000` as both keys
and values:

    ./bpt_dump_numbers test.db 0 100000

Insert `a` as key and `1` as value:

    ./bpt_cli test.db insert a 1

Change the value of `a` to `2`:

    ./bpt_cli test.db update a 2

Find the value of `a`:

    ./bpt_cli test.db search a

Find all values of keys between `100` to `10000`:

    ./bpt_cli test.db search 100 10000

License
-------

The MIT License (MIT)

Copyright (c) 2012 Zhao Cheng

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
