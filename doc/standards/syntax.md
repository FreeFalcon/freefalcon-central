# FreeFalcon C++ Syntax Standard

When multiple developers are collaborating on a project, it is important to
have a coding standard in place to ensure consistency throughout the code. It
is obvious when a source has been worked on by different people with
different coding styles; four different naming conventions used right next to
each other, horribly inconsistant syntax, and so on all make for developer
unhappiness. This standard strives to counter that amongst developers of
FreeFalcon.

This standard takes some inspiration from
[PEP-8](http://www.python.org/dev/peps/pep-0008/), considered the syntax
standard for all Python code. Many considerations need to be made for similar
rules to apply to C++, however, and that is what this standard does.

If you feel that you have a contribution to make to this standard, feel free
to send a pull request.

## Naming

Choose naming conventions according to the following:

 * If you are naming a namespace, class, or public method, it should be in
   PascalCase: words are separated by capitalizing the first letter of each
   word in the name, including the first letter.
 * A private or protected method is to be in PascalCase, but prefixed with a
   lower case m; such as mMyPrivateMethod(). Private and public method names
   can get somewhat similar, and this prevents any confusion.
 * The above two points should also be applied to member variables. Public
   member variables are to be named LikeThis, and private member variables
   mLikeThis.
 * Variable and function names outside of classes should be in
   lower_case_with_underscores.
 * Constants should be named in UPPER_CASE_WITH_UNDERSCORES.

In addition, follow these guidelines:

 * If a variable contains a measurement, include the units of measurement (in
   singular form- use Lb, not Lbs) at the end of the name; this helps prevent
   bad conversions. Examples- FuelWeightLb, MaxHeightFt, etc. Try to use
   imperial units unless it would be unusual to do so; metric may be
   technically better, by imperial units are used more often in the subject
   that this simulator covers, so use your best judgement in picking a sane
   unit to use for a particular value.
 * Do not make acronyms all-caps within names; instead, capitalize only the
   first letter as if it were any other word. Use MyAbcVar rather than
   MyABCVar. This creates a clearer separation between the last letter of the
   acronym and the first letter of the next word.
 * It should go without saying that variable names should be as relevant and
   descriptive as possible (while still maintaining some form of brevity-
   don't go around naming variables like AmountOfFuelRemainingInTheRightWing).
   If you are somewhat familiar with the source and cannot tell what a
   variable does, consider choosing a better name.

## Whitespace

Each level of indentation is to be four spaces; the hard tab character should
be avoided at all times.

Short statements should be separated by newlines at all times.

```C++
// Bad:
foo(); bar();

// Good:
foo();
bar();
```

Blank lines should be used to separate "ideas" in code, just as paragraphs
would separate ideas in prose. Reading a stream of nonstop code with no
whitespace breaking it up is comparable to reading a wall of text that isn't
split into paragraphs. It feels cluttered.

Generally, an if-else clause is going to be its own idea; in most cases,
whitespace should be put around the entire clause, but there should not be a
blank line in between an if and an else.

```C++
// Bad:
if (foo)
{
    // Code
}

else
{
    // Code
}

// Good:
if (foo)
{
    // Code
}
else
{
    // Code
}
```

A comment between the two is acceptable if it needs to be there, so long as
the comment is the only thing separating them.

Whitespace should not be used to align variable values when setting them;
leave them uneven at the ends. It is more concise in the long run.

```C++
// Bad:
int variable        = 5;
int differentLength = 8;

// Good:
int variable = 5;
int differentLength = 8;
```

## Line Length

Lines should be less than 80 characters (that is, no longer than 79 characters)
in length; this greatly helps out programmers that either have small screens
or wish to have multiple editing windows open at once, as well as making the
code more unified and orderly. Use any technique necessary to prevent lines
from spanning past 79 columns while maintaining cleanliness.

## Operators

Operators are to be padded with spaces:

```C++
// Bad:
foo=bar;
foo = 4*8;
if (foo<bar);

// Good:
foo = bar;
foo = 4 * 8;
if (foo < bar);
```

You can safely split lines on operators in order to keep line length in check,
but the operator should be at the end of the previous line.

```C++
// Bad:
foo = 1 + 2 + 3
      + 4 + 5;

// Good:
foo = 1 + 2 + 3 +
      4 + 5;
```

## Conditionals and Loops

Braces that define multi-line blocks of code are to be on their own separate
lines and on the same column as the brace that they match, inline with the
first character of the conditional declaration.

```C++
// Bad:
if (foo == bar) {
    foo();
    bar();
} else {
    bar();
    foo();
}

// Good:
if (foo == bar)
{
    foo();
    bar();
}
else
{
    bar();
    foo();
}
```

Note that they must have their own complete separate lines; this includes the
last while on a do-while loop.

```C++
// Bad:
do
{
    // Code
} while (foo);

// Good:
do
{
    // Code
}
while (foo);
```

Unrelated to syntax, the last item of a for loop declaration, if incremeting
the variable by one, should be a post-increment as opposed to a pre-increment.

```C++
// Bad:
for (int i = 0; i < 5; i++);

// Good:
for (int i = 0; i < 5; ++i);
```

While this isn't much of an issue in optimizing code with most compilers, it
ensures consistency throughout the code, as both methods are commonly seen.
In general, when there is a single increment that is done as its own statement,
it should be a post-increment.

Back on the topic of braces, loops and conditionals that have a single
statement should have none at all; simply indent the statement one level on
the next line.

Don't place the statement of a single line if statement on the same line.

```C++
// Bad:
if (foo == bar)
{
    myStatement();
}

if (foo == bar) myStatement();

// Good:
if (foo == bar)
    myStatement();
```

If the conditions of an if statement are too long, they can be made to span
several lines by lining them up with the previous statements (not the opening
paren). If && or || operators are at play, they should be at the end of the
previous line rather than the beginning of the current line.

```C++
// Good:
if (really && big && chain && of &&
    conditional && statements)
{
    // Code
}
```

There should be a single space separating the paren-enclosed statement from
the conditional/loop keyword.

```C++
// Bad:
if(foo == bar);

// Good:
if (foo == bar);
```

Single line else statements should always be on the same line as the else
keyword, as there is not that much space taken up, unless there is a comment
that you want for that line that wouldn't fit otherwise.

```C++
// Bad:
else
    singleStatement();

// All good:
else singleStatement();

else // Comment pertaining to this else statement
    singleStatement();
```

If you have a single line else statement, though, and the corresponding if
statement is many lines, consider switching the statements and inverting the
if clause so that the single line is less hidden.

## Functions and Methods

There should be no space between the name of a function and its
paren-enclosed parameters. Within the parameters, there should be spaces
after each comma separating the parameters.

```C++
// Bad:
myFunction (foo, bar);
myFunction(foo,bar);

// Good:
myFunction(foo, bar);
```

This goes for function declarations as well, as does the alignment of braces
on their own lines.

```C++
// Bad:
int myFunction(int foo, int bar) {
    // Code
}

// Good:
int myFunction(int foo, int bar)
{
    // Code
}
```

Everything in a function declaration before the parameters should be on a
single line; the parameters may be split to multiple lines if the line grows
too long.

```C++
// Bad:
int
myFunction(int foo, int bar)
{
    // Code
}

// All good:
int myFunction(int foo, int bar)
{
    // Code
}

int myFunction(big, list, of, parameters, that, dont, have, types,
               because, im, too, lazy, to, type, that, much)
{
    // Code
}
```

Because functions do not allow the omission of braces when there is only a
single statement within, any single statements should keep the same brace
formatting as if there were multiple statements. This also applies to
try-catch and any other similarly restricted blocks.

```C++
// Bad:
int myFunction(int foo, int bar)
    { iThinkICanGetAwayWithThis(); }

// Good:
int myFunction(int foo, int bar)
{
    noYouCantDoThisInstead();
}
```

If you want to pass a pointer to a function as a parameter, make it clear in
the function declaration that it is a pointer. It may seem more crazy looking,
but it makes it more syntactically obvious what the code does.

```C++
// Bad:
void myFunction(void paramFunction(int, int))
{
    paramFunction(0, 1);
}

// Good:
void myFunction(void (*paramFunction)(int, int))
{
    (*paramFunction)(0, 1);
}
```

In a function declaration, if no parameters are taken, it should be specified
as void. In function calls with no parameters, use empty parentheses.

```C++
// Bad:
int myFunction()
{
    // Code
}

// Good:
int myFunction(void)
{
    // Code
}
```

When calling a function, if the parameters are too long to fit on the line,
they may either be split and aligned, or completely separated from their
parentheses. See below:

```C++
// All good:
myFunctionCall(a, lot, of, parameters, being, called,
               it, needs, more, lines);

myFunctionCall(
    a, lot, of, parameters, being, called, it, needs, more, lines
);
```

## Arrays

When initializing a large array, split it to multiple lines like so:

```C++
// Good:
int nums[] = {
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8
}

// In that case, however, this is preferred:
int nums[] = {
    1, 2, 3, 4,
    5, 6, 7, 8
}
```

If you are desperate to save line space, you may even split up in this manner
when indexing an array:

```C++
// This is acceptable if necessary to stay under 80 characters:
longLineOfStuff->iNeedToSaveSomeSpace[
    soICanDoThis
]->andEvenMoreStuff();
```

## Classes

In a class declaration, public/protected/private keywords should be placed
in-line with the braces delimiting the class, with the rest at normal
indentation.

```C++
// Good:

class MyClass
{
public:
    string PublicStuff;
    MyClass(int moreStuff);

private:
    int mHowAboutSomePrivateStuff;
    string mAPrivateMethod(void);
};
```

A class declaration should be in a .h file, separate from its corresponding
.cpp file containing the method declarations. A .h file *should* only contain
one class, but if there are any small helper classes and you're sure that those
helpers are never used by anything outside of the corresponding .cpp file, you
may do that. If at any point the use of the helper class become necessary
elsewhere in the code, refactor it out to its own include file.

## Documentation

Documentation is an important part of code. You should document code *as it is
written*- do not code and then go back later to comment it. You won't.

### Comments

In general, excessive end of line comments should be avoided. Better to have
a comment on the line before a block of code rather than cluttering the block
itself.

```C++
here(); // It probably isn't a good idea to
are(); // Have comments on what each line is doing
some(); // Because that can inspire some sloppy
statements(); // Commenting.

// Instead, it is preferred to have a summary up here above the code that
// tells what the next few lines do.
here();
are();
some();
statements();
```

Use proper grammar and spelling in the comments; it makes things more
readable for everyone. There should be a space between the // and the
comment.

Comments should be descriptive and should NOT just be an English repetition
of the code that it is referring to. Let the code talk, and let the comments
give logic and reasoning.

```C++
foo();//This is bad
foo(); //This is better but still bad
foo(); // This is good

// this comment isn't capitalized... bad.
// There we go... nice and neat.
```

**Do not put C++ comments in C code.**

C++-style comments (```// ...``` format) in C files is wrong; do not do it.
Your Microsoft Visual C compiler may let this slip, but the vast majority of
compilers will cry if you do this. Use the ```/* ... */``` format if you don
t want your head taken off by somebody trying to test your code on another
compiler. C++ style comments are fine in C++ files, obviously.

Mind your comments in header files too; if the header file is included by a C
file, it had better use C-style comments. Includes are resolved before
comment stripping on most preprocessors, so don't use C++ comments in a
header file that is likely to be ever included by a C file (usually low level
stuff, use your judgement).

### Doc Comments

Doc comments are more formal than the inline comments used to mark code. They
are used either for large explanations of parts of code, or on a single line
to act as labels for different sections of something. They use two asterisks
in the opening and closing delimiters, as some syntax highlighters will see
them as documentation of higher importance that way.

```C++
/**
 * A multi-line doc comment should be in this format. Note the neat 'S' shape
 * formed by the delimiters at the top and bottom. If you're typing out more
 * than a few lines of comment to explain something, consider using this.
**/

// Below is a single line doc comment... notice how it appears to be a label.

/** Single Line **/
```

### Commenting Out Code

To comment out a large block of code, don't add a ```//``` to the beginning
of every line or use a ```/* */``` block to comment all of it out; the former
is messy and hard to uncomment without a regular expression, and the latter
will wreck any multiline comments inside of the block. Always use the C
preprocessor to comment out multiple lines of code:

```C++
#if 0
sprintf("This is now commented out.");
sprintf("It will not run.");
#endif
```

Don't use ```#ifndef notused``` or something like that; you know that the
second you do, somebody will quietly start defining ```notused``` elsewhere.
Just enclose the deactivated code in ```#if 0``` and ```#endif``` and you'll
be good to go.

This should really only ever be seen in feature branches; please avoid
checking commented out code into the main development branch. It is bad
practice to do this unless you have a very good reason, and confuses future
developers- the code is under source control for a reason, nothing will be
permanently lost, so if you're changing some code, commit to your decision
and get rid of the old code.

## Cleanliness

The opposite of not enough padding whitespace is too much padding; see the
examples below on avoiding excessive padding.

```C++
// Bad:
dontPadTheSemicolon() ;
function( noPaddingInsidePlease );
function(dont , pad , before , the , comma);

case :
    noNeedToPadColonsEither;

// Good:
dontPadTheSemicolon();
function(noPaddingInsidePlease);
function(dont, pad, before, the, comma);

case:
    noNeedToPadColonsEither;
```
