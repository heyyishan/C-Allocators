#include <stdio.h>

// 1. Define a typedef for the callback function pointer type
// This alias, 'CallbackFunc', represents a pointer to a function that 
// returns void and accepts one int parameter.
typedef void (*CallbackFunc)(int);

// 2. A function that takes a callback as an argument
void perform_action(int value, CallbackFunc callback) {
    printf("Inside perform_action function. Value is: %d\n", value);
    // Call the callback function through the pointer
    if (callback != NULL) {
        callback(value * 2); 
    }
}

// 3. The actual callback functions (implementations)
void my_callback_one(int result) {
    printf("  -> Inside my_callback_one. Result is: %d\n", result);
}

void my_callback_two(int result) {
    printf("  -> Inside my_callback_two. Result is: %d\n", result);
}

int main(void) {
    // 4. Declare variables of the new type and assign a function
    CallbackFunc cb_one = my_callback_one; // & is optional for function names
    CallbackFunc cb_two = my_callback_two;

    // 5. Pass the callback function pointers to another function
    printf("Calling perform_action with my_callback_one:\n");
    perform_action(10, cb_one);

    printf("\nCalling perform_action with my_callback_two:\n");
    perform_action(20, cb_two);

    return 0;
}

