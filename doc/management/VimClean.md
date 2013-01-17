# FreeFalcon Source Cleaning with VIM

Useful regular expression substitutions for cleaning up messy C++.
Run them in order for best results.

Move end-of-line comments to above the line:

    %s/\(\s*\)\([^ ].*\)\s*\(\/\/.*\)/\1\3\r\1\2/

S.G. loved to leave comments like this:

```C++
// 2002-08-12 ADDED BY S.G. ALL CAPS DESCRIPTION THAT IS A LITTLE TOO LONG
```

So, let's wrap them after the ```// YYYY-MM-DD _____ BY S.G.```:

    %s/\(\s*\)\/\/\s*\(\d\{4}-\d\d-\d\d\) \(.* \)\?S\.\?G\.\? \(.*\)/\1\/\/ \2 \3S.G.\r\1\/\/ \4

```C++
/*------------------------------------------------*/
/* There are also quite a few comments like this. */
/*------------------------------------------------*/
```

They are only one line and not extremely important, it's annoying.
Take care of them like so:

    %s/\(\s*\)\/\*-\+\*\/\n\s*\/\*\s*\(.*\)\s*\*\/\n\s*\/\*-\+\*\//\1\/\/ \2/

Fix the inline comments by single-spacing them, then capitalizing them.
Beware, the capitalization can mess with commented out code, if you care
about that then enable interactive and do it that way:

    %s/\/\/\( \{2,}\|\t\+\)\?\([^ ].*\)/\/\/ \2/g
    %s/\/\/\(\s*\)\(\w\)/\/\/\1\U\2/g

Capitalizing all of theme messes up me123's signature, which is meant to be
lower-case... sorry me123. We can fix it.

    %s/Me123/me123/I

Auto-wrap comments to keep them under 79 characters:

    %s/^\( \{4}\)\(\/\/.\{68}[^ ]*\)\( .*\)/\1\2\r\1\/\/\3/

After each run, add 4 to the first number and subtract 4 from the second
number until they stop matching. This will wrap them good enough.

Remove braces from single line blocks:

    %s/\s\+{\n\(.*\)\n\s\+}/\1/c

Interactive is ON for this, KEEP IT ON and MONITOR CAREFULLY. It's made so
that it shouldn't get any one line function definitions, but you still need
to be wary of try-catch, weirdly formed switches, etc.

Now concatenate the single line else statements onto one line:

    %s/\(\s*else\)\n\s*\(.*;\)/\1 \2/c

Interactive is recommended for this for safety. (There might be some weird
commented out line or whatever, who knows.)

Fix pointers from int* ptr to int *ptr:

    %s/\([^* /]\)\*\s\+\([^* ]\)/\1 *\2/gc

Recommend leaving interactive on. It's designed not to completely destroy
multi-line comments, but it could mess some up, and you never know.

Why do some people capitalize true and false? Beats me, fix it:

    %s/TRUE/true/g
    %s/FALSE/false/g

In fact, a few times I've even had to do this... talk about non-compliant:

    %s/BOOL/bool/g

Using extra whitespace to align equals signs is annoying. Flatten them.

    %s/\s\+=\s\+/ = /g

When casting to a pointer, make it look like (this*), not (this *):

    %s/(\([^* ]\+\) \*)/(\1*)/g

