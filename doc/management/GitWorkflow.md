# FreeFalcon Git Workflow

This document describes a set of rules to follow when using any FreeFalcon
Git repository, as well as instructions on how the FreeFalcon central
repository is set up and managed.

## FreeFalcon Central

The central repository should maintain an orderly history of commits, and
remain structured and organized. FreeFalcon uses the Git Flow commit model-
[click here](http://nvie.com/posts/a-successful-git-branching-model/) to learn
more, and see [this project](nvie/gitflow) for a tool to make using Git Flow
easy and intuitive.

## Git Rules

Follow these rules to prevent giving any confusion and headaches to other
developers.

 * **Never** rebase a commit that you have pushed to the repository. Ever. Just
   don't go there. There is never a case where this is a good idea. It messes
   with the history and screws everybody else up.
 * If you need to pull before pushing your commits because the remote has
   diverged and you cannot fast-forward, please do rebase instead of just doing
   a git pull. This will result in a cleaner commit log because it doesn't show
   an extra merge that doesn't need to be shown. This can be accomplished in
   one step via ```git pull --rebase```.
 * Remember that fixing merge conflicts on a rebase is not the same as doing so
   on a merge! After you fix the conflict, **do not commit**. Instead, run
   ```git rebase --continue``` to tell Git to commit your conflict resolution
   and finish rebasing.
 * The farthest back that you should ever ```git reset --hard``` is to the
   current state of the remote. Your status should always be ahead of or in
   sync with the remote- never behind! Resetting to behind the remote's head
   and then committing will change the history of the remote and screw up
   everybody else.
 * Don't make your commits massive! Each commit should only have one thing
   accomplished in it; this makes the history easier to read and revert, it
   makes bugs easier to track, and it makes merging easier for everyone. If
   you want to commit but have made several changes, carefully work the
   staging area so that you only commit one thing at a time. If there are many
   changes to vastly different things in the same file, well... you shouldn't
   have done that. Go ahead and commit it, and try not to do it again.
 * Just a tip: Push your commits often, but don't feel obligated to push every
   time that you commit. Keep enough commits to be able to test all of the
   changes that you've made together, and then push them all together once
   you're happy with your work.

## Commit Messages

Format your commit messages like so:

```
Short message describing the gist of the commit

A longer message describing the commit. This is written in normal
prose and should go over the changes that you made. Take up as much
space as you need here to describe it fully.

In this message, you can have as many paragraphs as you please. Also
feel free to use bullets:

 * Bullet point
 * Another bullet

Or even numbering:

 1. Numbered list
 2. Note how these lists are indented one space. If you need to wrap
    them, indent so that all of the text is in-line like this.
 3. Another list item
```

The short message on the first line should begin with a capital letter (unless the first word is a name or identifier beginning in lower case) and have no ending punctuation. It should very briefly describe the gist of the commit. The maximum length for this line is 50 characters.

Separated by a blank space from the first line is the longer message in paragraph format. The copy of Vim that Git will open for you has an additional configuration loaded into it that will automatically line wrap this paragraph for you, so keep typing without worry. If you use a different text editor and need to manually word wrap the lines, limit them to 72 characters.

If you commit is extremely trivial and can be fully described in the short commit message, you may forego the second paragraph.

## File Renaming Procedure

Renaming files can be very sensitive when there are multiple people working on the source. This procedure is an attempt to keep everybody from needing to search for a needle in a haystack after a file was renamed.

 * File an issue proposing the rename and why it should happen. Do your
   research and list all of the files that refer to the file that you are
   renaming so that they can be updated as well.
 * If the rename is approved, a notice will be made for all developers that are
   currently working on that file to get it to a point where it is ready to
   commit, so that the rename will take place on the most current version of
   the file as possible.
 * Whoever is in charge of the inbound repository will make sure that the file
   is up to date and then do the rename. Once this is complete, it will be
   updated on the inbound branch and a notice will be given that the rename is
   complete.
 * All branches should now update their copy of this file. This can be done by
   running ```git merge --no-commit --no-ff develop``` from their branch to
   pull in the changed file, then carefully editing the staging area so that
   the only change staged for commit is the file rename; then commit that,
   run ```git checkout -- .``` from the root of the source tree to discard all
   other changes that they pulled in from inbound, then continue working.

I agree that this is less than ideal; but, with Git, it is as good as file renaming is ever going to get. Still a nightmare though; avoid file renames by naming them something decent from the start and for the love of all that is good, don't misspell them. To prevent this headache, however, file names for new files are reviewed before they are merged into inbound to make sure that they are consistent and make sense.
