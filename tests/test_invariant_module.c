#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

START_TEST(test_recursion_depth_limit)
{
    // Invariant: The interpreter must not crash (SIGSEGV) on deeply nested input
    const char *payloads[] = {
        "(+ 1 2)",                          // Valid simple input
        "((((((((((1))))))))))",             // Moderate nesting (10 levels)
        NULL                                 // Placeholder for deep nesting
    };
    
    // Generate deeply nested payload (5000 levels)
    char deep_nested[12000];
    memset(deep_nested, '(', 5000);
    deep_nested[5000] = '1';
    memset(deep_nested + 5001, ')', 5000);
    deep_nested[10001] = '\0';
    
    const char *test_inputs[] = { payloads[0], payloads[1], deep_nested };
    int num_payloads = 3;

    for (int i = 0; i < num_payloads; i++) {
        int pipefd[2];
        ck_assert(pipe(pipefd) == 0);
        
        pid_t pid = fork();
        ck_assert(pid >= 0);
        
        if (pid == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execlp("./lone", "lone", NULL);
            _exit(127);
        }
        
        close(pipefd[0]);
        write(pipefd[1], test_inputs[i], strlen(test_inputs[i]));
        close(pipefd[1]);
        
        int status;
        waitpid(pid, &status, 0);
        
        // Must not crash with SIGSEGV
        if (WIFSIGNALED(status)) {
            ck_assert_msg(WTERMSIG(status) != SIGSEGV,
                "Interpreter crashed with SIGSEGV on payload %d (stack overflow)", i);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 10);

    tcase_add_test(tc_core, test_recursion_depth_limit);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}