////////////////////////////////////////////////////////////////////////////////
//
//  File           : cmpsc311-f16-assign1.c
//  Description    : This is the main source code for for the first assignment
//                   of CMPSC311, Spring 2020, at Penn State.
//                   assignment page for details.
//
//   Author        : Jonathan Mychack
//   Last Modified : 2/3/20
//

// Include Files
#include <stdio.h>
#include <string.h>

#define NUM_TEST_ELEMENTS 100

// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : round311
// Description  : rounds floating point numbers to nearest integer
//
// Inputs       : float_array - an array of floating point numbers
//                int_array - and array of integers to put the result in
//                array_size - size of the float_array and int_array, which are the same
// Outputs      : nothing

void round311(float *float_array, int *int_array, int array_size) {
    int i;
    for (i = 0; i < array_size; i++) {
        int leftOfDecimal = float_array[i];  //assigning a float to an int cuts off the decimal portion
        float rightOfDecimal = float_array[i] - leftOfDecimal;  //subtracting the original value from left of decimal gives the decimal itself
        if (rightOfDecimal >= 0.5) {
            leftOfDecimal += 1;
            int_array[i] = leftOfDecimal;
        }
        else {
            int_array[i] = leftOfDecimal;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : printfloatArr311
// Description  : prints all of the values, separated by commas, in an array of floating point numbers
//
// Inputs       : float_array - an array of floating point numbers
//                array_size - the size of the float_array
// Outputs      : nothing

void printfloatArr311(float *float_array, int array_size) {
    int i;
    for (i = 0; i < array_size; i++) {
        if (i == (array_size - 1)) {  //special case for last element so a comma doesn't get printed
            printf("%.2f", float_array[i]);
        }
        else {  //standard case
            printf("%.2f, ", float_array[i]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : printIntArr311
// Description  : prints the elements of an array of integers
//
// Inputs       : int_array - an array of integers
//                array_size - the size of the integer array
// Outputs      : nothing

void printIntArr311(int *int_array, int array_size) {
    int i;
    for (i = 0; i < array_size; i++) {
        if (i == (array_size - 1)) {  //special case so comma isn't printed after last element
            printf("%d", int_array[i]);
        }
        else {  //standard case
            printf("%d, ", int_array[i]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : bubbleSort311
// Description  : sorts the values of an array in ascending order by iterating through the
//                array repeatedly while comparing values two at a time and ascending larger
//                values
// Inputs       : int_array - an array of integer numbers
//                array_size - the size of the integer array
// Outputs      : nothing

void bubbleSort311(int *int_array, int array_size) {
    int i, j;
    for (i = 0; i < (array_size - 1); i++) {
        for (j = 0; j < (array_size - i - 1); j++) {  //for each iteration of i, one value gets put in place, so only have to iterate up to i for j's case
            if (int_array[j] > int_array[j + 1]) {
                int temp = int_array[j + 1];  //since we overwrite the value in [j+1], we need to store it temporarily
                int_array[j + 1] = int_array [j];
                int_array[j] = temp;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : toBinary
// Description  : converts decimal numbers to binary by using the conversion algorithm,
//                putting them into an array, appending any zeroes to the most-significant bit
//                if there aren't enough bits for the nibbles, and prints the elements of the array
//                in reverse order to account for the order the algorithm calculates the bits in
// Inputs       : value - the decimal value to be converted to binary
//
// Outputs      : nothing

void toBinary(int value) {
    int binary_array[32];
    int d_value = value;
    int i = 0, j;
    int count = 0;

    while (d_value > 0) {  //use > 0 as a check since the conversion algorithm ends with the decimal number reaching 0
        binary_array[i] = d_value % 2;
        d_value /= 2;
        i += 1;
    }

    if ((i + 1) % 4 != 0) {  //check for ability to separate value into nibbles
        int remainder = (i + 1) % 4;
        for (j = 0; j < remainder; j++) {
            binary_array[i + 1 + j] = 0;  //append zeroes to msb if there aren't enough bits for a nibble
            i += 1;
        }
    }

    for (; i >= 0; i--) {  //use a decrementing value to compensate for the reversed order of the array, i is the size of the array
        if ((count % 4 == 0) && (count != 0)) {  //after printing 4 bits, print a space
            printf(" ");
        }
        printf("%d", binary_array[i]);
        count += 1;
    }

    printf("\n");
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : countRange311
// Description  : calculates the number of values between a certain range of a inputted array
//                and returns it
// Inputs       : float_array - an array of floating point values
//                array_size - the size of the floating point array
//                min - the minimum of the range to calculate from
//                max - the maximum of the range to calculate from
// Outputs      : count - the magnitude of values within the range

int countRange311(float *float_array, int array_size, int min, int max) {
    int temp[100];
    int i;
    int count = 0;
    round311(float_array, temp, 100);
    for (i = 0; i < array_size; i++) {
        if ((temp[i] >= min) && (temp[i] <= max)) {  //check that the array element is between min and max
            count += 1;
        }
    }
    return count;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : histogram311
// Description  : creates a histogram using the data contained in the inputted array which
//                came from the countRange311 function
// Inputs       : int_array - an array of integers
//                array_size - the size of the integer array
// Outputs      : -1 if failure, 0 if success

int histogram311(int *int_array, int array_size) {
    int i, j;
    if (array_size != 10) {  //function fails if the array size is not 10
        return(-1);
    }
    else {
        printf("   +----------------------------------------+\n");  //header of graph

        for (i = 19; i >= 0; i--) {  //all of the rows with y-axis labels (0 to 19)
            printf("%.2d |", i);

            if ((i % 5) != 0) {  //case for y-values not divisible by 5 since they don't need "."'s printed

                for (j = 0; j < array_size; j++) {  //iterate through the array from countRange311 function

                    if ((int_array[j] / 3) >= i) {  //print xx if the count in the array is larger than the y-value, otherwise print spaces
                        printf(" xx ");
                    }
                    else {
                        printf("    ");
                    }
                }
                printf("|\n");
            }
            else {  //case for the y-values divisibe by 5, which need "."'s printed for visibility; works the same way

                for (j = 0; j < array_size; j++) {

                    if ((int_array[j] / 3) >= i) {
                        printf(".xx.");
                    }
                    else {
                        printf("....");
                    }
                }
                printf("|\n");
            }
        }
        printf("   +----------------------------------------+\n");  //footer of graph
        printf("     00  10  20  30  40  50  60  70  80  90");  //x-axis labels
    }
    return(0);  //if the function gets here, it succeeded so return the success value
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the CMPSC311 assignment #1
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main(int argc, char **argv) {

    /* Local variables */
    float f_array[NUM_TEST_ELEMENTS];
    int i_array[NUM_TEST_ELEMENTS], i;
    int hist_array[10];

    /* Preamble inforamtion */
    printf( "CMPSC311 - Assignment #1 - Spring 2020\n\n" );

    /* Step #1 - read in the float numbers to process */
    for (i=0; i<NUM_TEST_ELEMENTS; i++) {
        scanf("%f", &f_array[i]);
    }

    /* Step #2 - Alter the values of the float array as follows: all
    even numbered indices of the array should be multiplied by 0.78
    if the value is greater than 50 and 1.04 otherwise.  All odd valued
    indices should multiplied by 0.92 if the value is greater than 50
    and 1.15 otherwise */
    for (i = 0; i < NUM_TEST_ELEMENTS; i++) {
        if (i == 0) {  //special case for i = 0, treat it as even
            if (f_array[i] > 50) {
                f_array[i] *= 0.78;
            }
            else {
                f_array[i] *= 1.04;
            }
        }
        else if (i % 2 == 0) {  //case for evens where i != 0 and perform necessary calculations
            if (f_array[i] > 50) {
                f_array[i] *= 0.78;
            }
            else {
                f_array[i] *= 1.04;
            }
        }
        else {  //perform the operation for the remaining odd case
            if (f_array[i] > 50) {
                f_array[i] *= 0.92;
            }
            else {
                f_array[i] *= 1.15;
            }
        }
    }

    /* Step  #3 Round all of the values to integers and assign them
    to i_array using the round311 function */
    round311(f_array, i_array, NUM_TEST_ELEMENTS);

    /* Step #4 - Print out all of the floating point numbers in the
    array in order on a SINGLE line.  Each value should be printed
    with 2 digits to the right of the decimal point. */
    printf( "Testing printfloatArr311 (floats): " );
    printfloatArr311( f_array, NUM_TEST_ELEMENTS );
    printf("\n\n");

    /* Step #5 - Print out all of the integer numbers in the array in order on a SINGLE line. */
    printf( "Testing printIntArr311 (integers): " );
    printIntArr311( i_array, NUM_TEST_ELEMENTS );
    printf("\n\n");

    /* Step #6 - sort the integer values, print values */
    printf( "Testing bubbleSort311 (integers): " );
    bubbleSort311( i_array, NUM_TEST_ELEMENTS );
    printIntArr311( i_array, NUM_TEST_ELEMENTS );
    printf("\n\n");

    /* Step #7 - print out the last 5 values of the integer array in binary. */
    printf( "Testing toBinary:\n" );
    for (i=NUM_TEST_ELEMENTS-6; i<NUM_TEST_ELEMENTS; i++) {
        toBinary(i_array[i]);
    }
    printf("\n\n");

    /* Declare an array of integers.  Fill the array with a count (times three) of the number of values for each
    set of tens within the floating point array, i.e. index 0 will contain the count of rounded values in the array 0-9 TIMES THREE, the
    second will be 10-19, etc. */
    hist_array[0] = 3 * countRange311(f_array, 100, 0, 9);
    hist_array[1] = 3 * countRange311(f_array, 100, 10, 19);
    hist_array[2] = 3 * countRange311(f_array, 100, 20, 29);
    hist_array[3] = 3 * countRange311(f_array, 100, 30, 39);
    hist_array[4] = 3 * countRange311(f_array, 100, 40, 49);
    hist_array[5] = 3 * countRange311(f_array, 100, 50, 59);
    hist_array[6] = 3 * countRange311(f_array, 100, 60, 69);
    hist_array[7] = 3 * countRange311(f_array, 100, 70, 79);
    hist_array[8] = 3 * countRange311(f_array, 100, 80, 89);
    hist_array[9] = 3 * countRange311(f_array, 100, 90, 99);
    histogram311(hist_array, 10);

    /* Exit the program successfully */
    printf( "\n\nCMPSC311 - Assignment #1 - Spring 2020 Complete.\n" );
    return( 0 );
}
