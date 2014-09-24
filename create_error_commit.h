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

static void fail(const char *msg, const char *arg);
static void usage(const char *error, const char *arg);
static size_t is_prefixed(const char *arg, const char *pfx);
static uint32_t parse_shared(const char *shared);
git_signature* get_signature(git_repository *repo);
git_tree* get_working_dir(git_repository *repo);



/*
 ** 
 **
 ** Branch creation functions
 **
 **
 */


git_reference* _create_branch(git_repository *repo, const char* name, git_commit *target, char *message);
git_reference* create_branch(git_repository *repo, const char* name, const char* target_branch_name, char *message);

/*
 ** 
 **
 ** Commit creation functions
 **
 **
 */


int _create_commit(git_repository *repo, git_tree *tree_obj, const char *message,
 char *branch_name, const git_commit *parents[], size_t parent_count);

int create_commit(git_repository *repo, char *branch_name, const git_commit *parents[],
 git_tree *tree_obj, const char *message);


/*
 ** 
 **
 ** Higher-level functions for accessing and/or creating (in case they don't exist)
 ** various branches
 **
 **
 */

git_reference* get_error_branch(git_repository *repo);
git_reference* master_branch(git_repository *repo);
git_repository* current_repo();
int create_error_branch_commit(git_repository *repo, const char *message);