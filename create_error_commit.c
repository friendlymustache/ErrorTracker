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
const char* COMMIT_MESSAGE = "Attempting to add files to commit";

/*
 ** 
 **
 ** Helper functions for getting the necessary values/attributes for doing stuff with git
 **
 **
 */

static void fail(const char *msg, const char *arg)
{
	/* not actually good error handling */
	if (arg)
		fprintf(stderr, "%s %s\n", msg, arg);
	else
		fprintf(stderr, "%s\n", msg);
	exit(1);
}

static char* build_commit_message(const char *message, int strip_comments, char comment_char) {
	// Get a pointer to a git buffer
	git_buf *output = (git_buf * ) malloc(sizeof(git_buf));
	output->ptr = NULL;
	output->size = 0;
	output->asize = 0;

	const git_error *e;

	int prettify_result = git_message_prettify(output, message, 0, 'a');
	switch(prettify_result) {
		case 0:
			break;
		default:
			e = giterr_last();
			printf("Method 'build_commit_message' failed with error code %d and message %s\n", prettify_result, e->message);
	}

	char *prettified_message = output->ptr;
	free(output);
	return prettified_message;
}

static void usage(const char *error, const char *arg)
{
	fprintf(stderr, "error: %s '%s'\n", error, arg);
	fprintf(stderr, "usage: init [-q | --quiet] [--bare] "
			"[--template=<dir>] [--shared[=perms]] <directory>\n");
	exit(1);
}

static size_t is_prefixed(const char *arg, const char *pfx)
{
	/* simple string prefix test used in argument parsing */
	size_t len = strlen(pfx);
	return !strncmp(arg, pfx, len) ? len : 0;
}

static uint32_t parse_shared(const char *shared)
{
	/* parse the tail of the --shared= argument */
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

// static void create_initial_commit(git_repository *repo);

git_signature* get_signature(git_repository *repo) {
	/* Returns git signature for creating commits on the given repo */
	git_signature *sig;
	if (git_signature_default(&sig, repo) < 0)
		fail("Unable to create a commit signature.",
			 "Perhaps 'user.name' and 'user.email' are not set");
	return sig;
}


git_tree* get_working_dir(git_repository *repo) {
	/* 
	 * Returns a git_tree object containing the contents of the working directory 
	 * (of the current branch) of the provided repo
	 */	
	git_index* index_obj;
	git_tree *tree_obj = NULL;
	git_oid *tree_oid = (git_oid *) malloc(sizeof(git_oid));
	const git_error *e;
	// Load current repo's index into the index_obj variable

	int error = git_repository_index(&index_obj, repo);
	switch(error) {
		case 0:
			printf("Successfully loaded/created index object\n");
			break;
		default:
			e = giterr_last();
			printf("Error %d/%d: %s\n", error, e->klass, e->message);		
	}	

	// See http://git.kaarsemaker.net/libgit2/blob/5173ea921d4ccbbe7d61ddce9a0920c2e1c82035/tests-clar/index/addall.c
	// Add files to index
	
	git_strarray paths = {NULL, 0};
	char* strs[1];
	strs[0] = "*";
	paths.strings = strs;
	paths.count   = 1;

	int add_result = git_index_add_all(index_obj, &paths, 0, NULL, NULL);
	// int add_result = git_index_read(index_obj, 1);

	switch(add_result) {
		case 0:
			printf("Successfully added working directory files to index object\n");
			break;
		default:
			e = giterr_last();
			printf("Error %d/%d: %s\n", error, e->klass, e->message);
		}
	

	// Write index to tree

	
	int tree_conversion = git_index_write_tree(tree_oid, index_obj);
	printf("Done attemping to write to tree\n");
	switch(tree_conversion) {
		case 0:
			printf("Successfully wrote tree \n");
			break;
		default:
			printf("Failed to write tree from index, error code: %d\n", tree_conversion);
	}

	// Free index
	git_index_free(index_obj);


	// Look up the tree object using tree_oid 
	printf("About to lookup tree\n");
	int tree_lookup = git_tree_lookup(&tree_obj, repo, tree_oid);

	switch(tree_lookup) {
		case 0:
			printf("Successfully looked-up tree\n");
			break;
		default:
			e = giterr_last();
			printf("Error %d/%d: %s\n", tree_lookup, e->klass, e->message);		
	}
	// Free tree oid
	free(tree_oid);
	return tree_obj;
}


/*
 ** 
 **
 ** Branch creation functions
 **
 **
 */

git_reference* _create_branch(git_repository *repo, const char* name, git_commit *target, char *message) {

	/* Helper/more general function: 
	 * Returns the resulting reference after creating a branch on the given repo with the given name and
	 * message based off of the given target commit
	 */

	git_reference* output_reference;
	// Set force to 1 to overwrite existing branch with same name in the case of a collision
	int force = 0;
	git_signature *signature = get_signature(repo);
	git_branch_create(&output_reference, repo, name, target, force, signature, message);
	return output_reference;
}


git_reference* create_branch(git_repository *repo, const char* name, const char* target_branch_name, char *message) {

	/* Creates a new branch using the name (name),
	 * based off of the HEAD commit of the target branch with the
	 * provided name (target_branch_name)
	 */


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


/*
 ** 
 **
 ** Commit creation functions
 **
 **
 */


int _create_commit(git_repository *repo, git_tree *tree_obj, const char *message, char *branch_name, const git_commit *parents[], size_t parent_count) {


	/* Helper function for creating a commit on the repository represented by <repo> */
	printf("DEBUG: In _create_commit\n");

	// Get commit signature
	git_signature *signature = get_signature(repo);
	// Set author and committer signatures to the commit signature
	git_signature *author = signature;
	git_signature *committer = signature;
	git_oid *commit_oid = (git_oid *) malloc(sizeof(git_oid));
	const git_error *e;

	// ------------------------------------------- TEMP debug stuff so that parent count is set to 0 -----------------------------------       //
	// parent_count = 0;
	// parents = NULL;

	printf("DEBUG: _create_commit: Ready to call git_commit_create with %zu parents \n", parent_count);
	
	int commit_result = git_commit_create(commit_oid, repo, branch_name, author, committer,
		NULL, message, tree_obj, parent_count, parents);


	switch(commit_result) {
		case 0:
			printf("DEBUG - _create_commit: Successfully created commit\n");
			break;
		default:
			e = giterr_last();
			printf("_create_commit failed with error %d/%d: %s\n", commit_result, e->klass, e->message);		
	}

	free(commit_oid);
	return commit_result;

}


int create_commit(git_repository *repo, char *branch_name, const git_commit *parents[], git_tree *tree_obj, const char *message) {

	/* Creates a commit on the given repo using the provided index object and
	 * the provided message
	 */

	size_t parent_count = sizeof(parents) / sizeof(git_commit*);
	int result = _create_commit(repo, tree_obj, message, branch_name, parents, parent_count);
	if(result != 0) {
		fail("create_commit failed", NULL);
	}	
	else {
		printf("create_commit succeeded\n");
	}	
	return result;
}


/*
 ** 
 **
 ** Higher-level functions for accessing and/or creating (in case they don't exist)
 ** various branches
 **
 **
 */

git_reference* get_error_branch(git_repository *repo)
{
	/* Returns a reference object representing the _error branch in repo
	 * represented by the provided *repo argument
	 */	
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



int create_error_branch_commit(git_repository *repo, const char *message) {
	/* Creates a commit on the _error branch of the given repository using the current
	 * working directory of the master branch
	 */	


	printf("DEBUG - create_error_branch_commit: Getting working tree from repo\n");
	git_tree *working_tree;
	working_tree = get_working_dir(repo);
	printf("DEBUG - create_error_branch_commit: Got working tree from repo\n");

	// Get a reference to the head commit of the error branch and look up its oid
	// Branches are just named references to commits, so looking up the error_branch
	// reference should give us a reference to its head
	const git_reference* error_branch_reference = get_error_branch(repo);
	const git_oid* error_branch_oid = git_reference_target(error_branch_reference);
	printf("DEBUG - create_error_branch_commit: Got error branch\n");

	// Initialize a commit object to store the error branch's head commit (the parent of the
	// commit we're about to create)
	git_commit* parent_commit;
	// Store the error branch's head commit in the abovementioned commit object
	int commit_lookup_result = git_commit_lookup(&parent_commit, repo, error_branch_oid);

	if(commit_lookup_result != 0) {
		fail("create_error_branch_commit failed at commit_lookup_result", NULL);
	}	
	else {
		printf("DEBUG - create_error_branch_commit: Looked up error branch head commit\n");
	}

	// Initialize the array of parent commits for the commit we're creating
	const git_commit* parents[1] = { (const git_commit*) parent_commit };

	// Create our commit
	int error_branch_commit_create_result;
	char* ref = "refs/heads/_error";
	char *prettified_message = build_commit_message(message, 0, 'a');

	error_branch_commit_create_result = create_commit(repo, ref, parents, working_tree, message);
	if(error_branch_commit_create_result != 0) {
		fail("create_error_branch_commit failed at create_commit", NULL);
	}	
	else {
		printf("DEBUG - create_error_branch_commit: succeeded with result %d\n", error_branch_commit_create_result);
	}

	git_commit_free(parent_commit);
	git_tree_free(working_tree);
	printf("Done freeing stuff\n");
	return error_branch_commit_create_result;
}

int create_error(const char *message) {
	git_repository *repo = current_repo();
	create_error_branch_commit(repo, message);
}