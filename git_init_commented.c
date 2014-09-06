/* If you compile and run this program, you'll get output something like this:

	$ ./init --initial-commit ./foo
	Initialized empty Git repository in /tmp/foo/
	Created empty initial commit
	$ cd foo
	$ git log
	commit 156cebb6100829b61b757d7ade498b664d20ee4b
	Author: Ben Straub <bs@github.com>
	Date:   Sat Oct 5 20:59:50 2013 -0700

	    Initial commit
*/	  




/* Code URL:
 * https://libgit2.github.com/docs/examples/init/ 
 */

#include <stdio.h>
#include <stdlib.h>
#include <git2.h>


static void create_initial_commit(git_repository *repo)
{

	/* First, we declare all of our variables, which might give you
	 * a clue as to what's coming.
	 */
    git_signature *sig;
    git_index *index;
    git_oid tree_id, commit_id;
    git_tree *tree;

    /* Next, we generate a commit signature using the values in the
     * user's config, and timestamp of right now.
     */

    if (git_signature_default(&sig, repo) < 0)
        fatal("Unable to create a commit signature.",
              "Perhaps 'user.name' and 'user.email' are not set");    

    /* Now we store the index's tree into the ODB to use for the commit.
     * Since the repo was just initialized, the index has an empty tree.
     */

     // Loads repository index into the index* pointer. Remember, the index
     // is the staging area
    if (git_repository_index(&index, repo) < 0)
        fatal("Could not open repository index", NULL);

    // Writes the object-id (SHA-1 hash) of the index's tree to the tree_id
    // pointer
    if (git_index_write_tree(&tree_id, index) < 0)
        fatal("Unable to write initial tree from index", NULL);

    git_index_free(index);

    /* It's worth noting that this doesn't actually write the index to disk.
     * There's a separate call for that: git_index_write. All this code does is
     * use the empty index to get the SHA-1 hash of the empty tree.

	/* Okay, now we have the empty tree's SHA-1 hash, but we need an actual
	 * git_tree object to create a commit.
	 */

	// Loads the tree within the repo provided by <repo>
	// containing the given SHA-1 hash (object id) into the tree*
	// pointer
    if (git_tree_lookup(&tree, repo, &tree_id) < 0)
        fatal("Could not look up initial tree", NULL);

	/* Now we're ready to write the initial commit. Normally you'd look up HEAD
	 * to use as the parent, but this commit will have no parents.
	 */

    if (git_commit_create_v(
            &commit_id, repo, "HEAD", sig, sig,
            NULL, "Initial commit", tree, 0) < 0)
        fatal("Could not create the initial commit", NULL);

    /* And (of course) clean up our mess. */
    git_tree_free(tree);
    git_signature_free(sig);
}    	 

int main(int argc, char *argv[])
{

	/* Let's skip the frontmatter, and jump straight to the meat of this thing.
	 * The main function starts off with some boilerplate and a bit of command-line
	 * parsing noise, which we'll ignore for the purposes of this article.
	 */	
    git_repository *repo = NULL;

    struct opts o;
   	o = { 1, 0, 0, 0, GIT_REPOSITORY_INIT_SHARED_UMASK, 0, 0, 0 };

    git_threads_init();
    parse_opts(&o, argc, argv);

	/* Next we have a bit of a shortcut; if we were called with no options, we can
	 * use the simplest API for doing this, since its defaults match those of the
	 * command line. That check_lg2 utility checks the value passed in the first
	 * parameter, and if it's not zero, prints the other two parameters and exits
	 * the program. Not the greatest error handling, but it'll do for these examples.
	 */
 	if (o.no_options) {
        check_lg2(git_repository_init(&repo, o.dir, 0),
            "Could not initialize repository", NULL);
    }


	/* If the situation is more complex, you can use the extended API to handle it.
	 * The fields in the options structure are designed to provide much of what git
	 * init does. Note that it's important to use the _INIT structure initializers;
	 * these structures have version numbers so future libgit2's can maintain
	 * backwards compatibility.	
	 */

    else {
        git_repository_init_options initopts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
        initopts.flags = GIT_REPOSITORY_INIT_MKPATH;

        if (o.bare)
            initopts.flags |= GIT_REPOSITORY_INIT_BARE;

        if (o.template) {
            initopts.flags |= GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
            initopts.template_path = o.template;
    }    

	/* Libgit2's repository is always oriented at the .git directory, so specifying
	 * an external git directory turns things a bit upside-down:
	 */

    if (o.gitdir) {
        initopts.workdir_path = o.dir;
        o.dir = o.gitdir;
    }

    if (o.shared != 0)
        initopts.mode = o.shared;    


	/* Now the call that does all the work: git_repository_init_ext.
 	 * The output (if the call succeeds) lands in repo, which is a git_repository*,
 	 * which we can then go on and use.
 	 */
    check_lg2(git_repository_init_ext(&repo, o.dir, &initopts),
                "Could not initialize repository", NULL);
    }

    if (!o.quiet) {
        if (o.bare || o.gitdir)
            o.dir = git_repository_path(repo);
        else
            o.dir = git_repository_workdir(repo);

        printf("Initialized empty Git repository in %s\n", o.dir);
    }

	/* If we get this far, the initialization is done. This example goes one step
	 * farther than git, by providing an option to create an empty initial commit.
	 * The body of this function is below.
	 */
	
	if (o.initial_commit) {
        create_initial_commit(repo);
        printf("Created empty initial commit\n");
    }

	/* Now to clean up our mess; C isn't your mother. Unless the docs specifically
	 * say otherwise, any non-const pointer that's filled in by libgit2 needs to be
	 * freed by the caller
	 */
    git_repository_free(repo);
    git_threads_shutdown();

    return 0;       
}		