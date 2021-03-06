What is it?
-----
quickerd stands for quick ER Diagram.
It converts an input text file written in a simple syntax to a graphviz input file which can then be used with graphviz to generate the ERD.

Why is it ?
----
Well, commercial programs used for ERD drawing are stuffed with loads of unnecessarily confusing tools and makes the job of drawing  a simple ERD a PITA.  
Hence, quickerd.  
You write a ERD description file (be there in a minute), pass it through quickerd, pass the output file of quickerd to graphviz. Done.
No dragging, no scaling, no copying, no messing. 

How to compile ?
--
Get the source. Extract.  
Run `make` in the source directory to compile. The output binary is named 'quickerd'.  
Of course, you'll also need to install [graphviz](http://www.graphviz.org/Download_windows.php) to render the ERD.

How to use ?
---
To use quickerd you first have to write a ERD description file.
A ERD descr. file contains three basic elements - 
* Variable declaration / definition
* Table specification
* Relationship Specification

**Variable Declaration and Definition**  
*syntax*:`var_name : value`

**Table Specification**  
*syntax*: `table name`(`col1`[,`col2`,...])  

**Relationship Specification**  
*syntax* `table1`>`table2`,`relationship name`,`cardinality`

*e.g.*
```
employee(eid,dpt_id,name,sex,age)
salary(eid,salary)
#
employee>salary,is paid,1:1
```
is a valid ERD description file specifying two tables: employee and salary. It also specifies that table employee has a 1 to 1 relation to the table salary which is named 'is paid'.
Lines beginning with '#' are ignored.

Starting from quickerd v0.2, variable substitution is supported. The above description could also be written as
```
emp : employee
sal : salary
emp (eid, dpt_id, name,sex, age)
sal (eid, salary)
#
empl > sal, is paid, 1:1
```
As simple as *that*.

Once you have the description file (say erd.txt) ready, run quickerd as
```
quickerd erd.txt out.gv
```
`out.gv` is your input file for graphviz. Now, to generate the actual graph i.e. the ERD, run
```
dot -Tpng out.gv > out.png
```
This will save the ERD in png format to out.png .
#####If you're on Windows, open `out.gv` in [graphviz](http://www.graphviz.org/Download_windows.php).  
This looks like:  ![This looks like :](https://raw.githubusercontent.com/0pointr/quickerd/master/Examples/simple.png)  
See other examples [here](https://github.com/0pointr/quickerd/tree/master/Examples).  

---
#### Can I copy/modify/distribute ?
Yes, of course, provided you keep the original copyright information intact.  
___
* Notes
  * Syntax checking is rigid, in the sense that no part of a specification may be omitted. On error, quickerd will print the line of error and exit.
    Except for variable names, there can be arbrtitary number of space/s within and around a string. The spaces within will be preserved.
  * Other than syntax errors, quickerd will also whine if an undefined table is used in a relationship.
  * Variables have no specific identifiers that identify them spcecifically as veriables. Thus any string not already defined as a variable is used as a literal
    string and classified according to its context.
  * Feature wise, quickerd is rather lacking. It doesn't provided all the symantics the ERD specification allows. It works well for small needs but could use some more love.
