/*
 * This is a sample program that attempts to create a commit on the _error branch
 * using the current contents (index) of the 'master' branch.
 */

#include <stdio.h>
#include <git2.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
const char* ERROR_BRANCH_NAME = "_error";
const char* MASTER_BRANCH_NAME = "master";


/* not actually good error handling */
static void fail(const char *msg, const char *arg)
{
	if (arg)
		fprintf(stderr, "%s %s\n", msg, arg);
	else
		fprintf(stderr, "%s\n", msg);
	exit(1);
}

static void usage(const char *error, const char *arg)
{
	fprintf(stderr, "error: %s '%s'\n", error, arg);
	fprintf(stderr, "usage: init [-q | --quiet] [--bare] "
			"[--template=<dir>] [--shared[=perms]] <directory>\n");
	exit(1);
}

/* simple string prefix test used in argument parsing */
static size_t is_prefixed(const char *arg, const char *pfx)
{
	size_t len = strlen(pfx);
	return !strncmp(arg, pfx, len) ? len : 0;
}

/* parse the tail of the --shared= argument */
static uint32_t parse_shared(const char *shared)
{
	if (!strcmp(shared, "false") || !strcmp(shared, "umask"))
		return GIT_REPOSITORY_INIT_SHARED_UMASK;

	else if (!strcmp(shared, "true") || !strcmp(shared, "group"))
		return GIT_REPOSITORY_INIT_SHARED_GROUP;

	else if (!strcmp(shared, "all") || !strcmp(shared, "world") ||
			 !strcmp(shared, "everybody"))
		return GIT_REPOSITORY_INIT_SHARED_ALL;

	else if (shared[0] == '0') {
		long val;
		char *end = NULL;
		val = strtol(shared + 1, &end, 8);
		if (end == shared + 1 || *end != 0)
			usage("invalid octal value for --shared", shared);
		return (uint32_t)val;
	}

	else
		usage("unknown value for --shared", shared);

	return 0;
}

/* forward declaration of helper to make an empty parent-less commit */
static void create_initial_commit(git_repository *repo);

// Returns git signature for creating commits on the given repo
git_signature* get_signature(git_repository *repo) {
	git_signature *sig;
	if (git_signature_default(&sig, repo) < 0)
		fail("Unable to create a commit signature.",
			 "Perhaps 'user.name' and 'user.email' are not set");
}


/* Helper/more general function: 
 * Returns the resulting reference after creating a branch on the given repo with the given name and
 * message based off of the given target commit
 */
git_reference* _create_branch(git_repository *repo, const char* name, git_commit *target, char *message) {
	git_reference* out;
	// Set force to 1 to overwrite existing branch with same name in the case of a collision
	int force = 0;
	git_signature *signature = get_signature(repo);
	git_branch_create(&out, repo, name, target, force, signature, message);
	return out;
}

/* Main function - creates a new branch using the name (name),
 * based off of the HEAD commit of the target branch with the
 * provided name (target_branch_name)
 */
git_reference* create_branch(git_repository *repo, const char* name, const char* target_branch_name, char *message) {
	/* Pointer for storing a reference to the target branch */
	git_reference* target_reference;
	/* Pointer for storing a reference to the HEAD commit of the target branch */
	git_reference* target_branch_head_reference;
	/* Pointer for storing the actual HEAD commit object of the target branch */
	git_commit* target;
	/* Pointer to the oid of the HEAD commit of the target branch */
	const git_oid* target_oid;

	/* Load a reference to the target branch into the target_reference pointer */
	switch(git_branch_lookup(&target_reference, repo, target_branch_name, GIT_BRANCH_LOCAL)) {
		case GIT_ENOTFOUND:
			printf("Failed to find branch with name %s\n", target_branch_name);
			break;
		default:
			/* Get the HEAD commit of the branch referenced by target_reference. This loads a 
			 * git_reference object (that we still need to convert into a git_commit object) into
			 * the target_branch_head_reference pointer
			 */
			git_repository_head(&target_branch_head_reference, repo);		
			/* Look up the actual HEAD commit object from the target branch using the oid contained
			 * in the target_branch_head_reference object - we get this using the git_reference_target
			 * API method
			 */
			target_oid = git_reference_target(target_branch_head_reference);
			git_commit_lookup(&target, repo, target_oid);		
			/* Use _create_branch helper function to 
			 * create a branch based off the HEAD commit of the target branch
			 */
			_create_branch(repo, name, target, message);			 
	}
}



git_reference* error_branch(git_repository *repo)
{
	git_reference* out = NULL;
	/* Find the _error branch used to track runtime errors */
	switch(git_branch_lookup(&out, repo, ERROR_BRANCH_NAME, GIT_BRANCH_LOCAL)) {
		/* If the branch doesn't exist, create it */
		case GIT_ENOTFOUND:
			printf("Could not find branch with name %s, creating it now...\n", ERROR_BRANCH_NAME);
			out = create_branch(repo, ERROR_BRANCH_NAME, MASTER_BRANCH_NAME, "Creating error branch dawg\n");
			printf("Done.\n");
			break;
		/* If an 'invalid spec' error occurred, print the error and exit the program without doing other stuff */
		case GIT_EINVALIDSPEC:
			printf("Invalid spec error, see https://libgit2.github.com/libgit2/#HEAD/group/branch/git_branch_lookup\n");
			break;
		default:
			printf("Successfully acquired error branch\n");
	}
	return out;
}

git_reference* master_branch(git_repository *repo) {
	git_reference* out;
	if(git_branch_lookup(&out, repo, MASTER_BRANCH_NAME, GIT_BRANCH_LOCAL) != 0) {
		fail("Failed to find branch with name", MASTER_BRANCH_NAME);
	}
	printf("Successfully acquired master branch\n");
	return out;	
}

git_repository* current_repo() {
	git_repository* out;
	char* path = ".git";
	switch(git_repository_open(&out, path)) {
		case GIT_ENOTFOUND:
			printf("Failed to open git repository at current directory - run git init to make sure one exists\n");
			break;
		default:
			printf("Successfully acquired current repo\n");
	}
	return out;
}

git_index* get_working_dir() {
	git_index* index_obj;

	// Initialize index object
	int result = git_index_new(&index_obj);
	switch(result) {
		case 0:
			printf("Successfully initialized new index object\n");
			break;
		default:
			printf("Failed to initialize new index object, error code: %d\n", result);
	}

	// See http://git.kaarsemaker.net/libgit2/blob/5173ea921d4ccbbe7d61ddce9a0920c2e1c82035/tests-clar/index/addall.c
	git_strarray paths = {NULL, 0};
	int add_result = git_index_add_all(index_obj, &paths, 0, NULL, NULL);
	switch(add_result) {
		case 0:
			printf("Successfully added working directory files to index object\n");
			break;
		default:
			printf("Failed to add working directory files to index object, error code: %d\n", add_result);
	}
}

int main(int argc, char *argv[])
{
	git_repository *current;
	git_reference *master;
	git_reference *error;
	git_index *working_dir;

	current = current_repo();
	master = master_branch(current);
	error = error_branch(current);
	working_dir = get_working_dir();
	return 0;	
}

/* Unlike regular "git init", this example shows how to create an initial
 * empty commit in the repository.  This is the helper function that does
 * that.
 */
static void create_initial_commit(git_repository *repo)
{
	git_signature *sig;
	git_index *index;
	git_oid tree_id, commit_id;
	git_tree *tree;

	/* First use the config to initialize a commit signature for the user */

	if (git_signature_default(&sig, repo) < 0)
		fail("Unable to create a commit signature.",
			 "Perhaps 'user.name' and 'user.email' are not set");

	/* Now let's create an empty tree for this commit */

	if (git_repository_index(&index, repo) < 0)
		fail("Could not open repository index", NULL);

	/* Outside of this example, you could call git_index_add_bypath()
	 * here to put actual files into the index.  For our purposes, we'll
	 * leave it empty for now.
	 */

	if (git_index_write_tree(&tree_id, index) < 0)
		fail("Unable to write initial tree from index", NULL);

	git_index_free(index);

	if (git_tree_lookup(&tree, repo, &tree_id) < 0)
		fail("Could not look up initial tree", NULL);

	/* Ready to create the initial commit
	 *
	 * Normally creating a commit would involve looking up the current
	 * HEAD commit and making that be the parent of the initial commit,
	 * but here this is the first commit so there will be no parent.
	 */

	if (git_commit_create_v(
			&commit_id, repo, "HEAD", sig, sig,
			NULL, "Initial commit", tree, 0) < 0)
		fail("Could not create the initial commit", NULL);

	/* Clean up so we don't leak memory */

	git_tree_free(tree);
	git_signature_free(sig);
}