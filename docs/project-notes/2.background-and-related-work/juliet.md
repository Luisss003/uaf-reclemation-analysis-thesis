# Juliet 
This is a test suite for testing the capabiltiies of static analysis tools. A test case is simply a buildable code snippet that has exactly one type of flaw.

We need a sample set representative of different types of UAF executions so we need to refer to `Section 3: Test Case Naming` to determine how to find those different variations/representations:
1. Each testcase is associated with one CWE entry.
2. The testcase filename tells us 4 things:
    1. ID number and sometimes shortened CWE name
    2. Function variant name.
    3. Two digit num indicating flow variant, indicating type of data and or control flow used in testcase
    4. programming lang used via the extension i.e. `.c, .h, etc`
3. A function variant simply indicates the type of function or data type involved in the bug.
    1. It can additionally contain a `key string`, to indicate test case characteristics to keep in mind when building the test case i.e. `w32`, which tells us that this test case is specific to Windows.
4. Each test case comes with a `flow variant`, since the idea is that your tool should be able to follow the correct control flow to find a tool, discarding non vulnerable paths.
    1. `01` don't contain any added control or data flow complexity. These are best for baseline
    2. `02` to `22`: these contain a mix of control and data flow patterns. 

Refer to appendix C in the juliet user guide to see the different possible types of control/data flow variants.

Here is an example testcase filename `CWE78_OS_Command_Injection__char_listen_socket_w32_execv_41.c.`