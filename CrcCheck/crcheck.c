#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define BUFFER_SIZE 513

// Function prototypes
bool read_data(char *txtFile, char buffer[BUFFER_SIZE]); // Function to read in the txt file data
uint32_t xor(uint32_t a, uint32_t b); // Simple XOR function per specs
int getMS1B(uint32_t a); // Helper function to get the position of the highest set bit in the given number
uint32_t crc(char *buffer, int start, int length, uint32_t polynomial, uint32_t *initialization); // CRC calculation function
void crc_mode_c(char *txtFile);
void crc_mode_v(char *txtFile);


int main(int argc, char **argv)
{
	// Check for missing parameters
	if (argc < 3)
	{
		fprintf(stderr, "Error.  Missing parameters.");
		return 0;
	}

	// Check if the length of the 1st argument is greater than 1
	// Check if the 1st index (0) of the 1st argument is 'c' or 'v'
	// If none of these things are true, throw error
	if (strlen(argv[1]) > 1 || !(argv[1][0] == 'c' || argv[1][0] == 'v'))
	{
		fprintf(stderr, "Error. Invalid parameters.  Please enter either 'c' or 'v'\n");
		return 0;
	}
	char *txtFile = argv[2];

	if (argv[1][0] == 'c')
	{
		crc_mode_c(txtFile);
	}

	if (argv[1][0] == 'v')
	{
		crc_mode_v(txtFile);
	}

	return 0;
}

// Function definitions
bool read_data(char *txtFile, char buffer[BUFFER_SIZE])
{
	FILE *pTxtFile = fopen(txtFile, "r");

	if (pTxtFile == NULL)
	{
		fprintf(stderr, "Error opening file.");
		return 0;
	}

	int count = fread(buffer, 1, 512, pTxtFile);

	// Print out each line of 64 bits of the unformatted text file
	printf("CRC-15 Input text from file\n\n");

	for (int i = 0; i < count; i += 64)
	{
		int length = 64;
		if (i + length > count) { length = count - i; }

		char temp = buffer[i + length];
		buffer[i + length] = 0;
		printf("%s\n", buffer + i);
		buffer[i + length] = temp;
	}

	while (count < 504)
	{
		buffer[count++] = '.'; // Add the .s
	}
	buffer[512] = '\0'; // Append a null terminator to the end of the file

	return true;
}

uint32_t xor (uint32_t a, uint32_t b)
{
	return a ^ b;
}

// Helper function to get the position of the highest set bit in the given number
int getMS1B(uint32_t a)
{
	// Input is 32 bits long, so bit 31 is the highest possible
	int p = 31;
	// We're checking the p-th bit, as long as it's 0, we continue decrementing p
	// When the bit is set, we exit the loop because then we've found the highest 1 bit
	while ((a & (1 << p)) == 0 && p >= 0)
	{
		p--;
	}
	return p;
}

// Calculate the crc value of a block of data
// Buffer is all input data
// Start is the index in buffer where to start
// Length is the length of the block of data to use
// Polynomial is the crc polynomial
/* Initialization is an in/out parameter that is the starting value for the crc register. 
it allows computing the full crc of a block in buffer and then continuing the calculation with the next block 
by passing the same variable in*/
uint32_t crc(char *buffer, int start, int length, uint32_t polynomial, uint32_t *initialization)
{
	// CRC register, restore previous crc calculation
	uint32_t crc = *initialization;
	// Position of most significant 1 in the polynomial
	int polynomialMS1B = getMS1B(polynomial);
	// Only 1 bit in this number is set: the highest 1 of the polynomial. If this bit is set in the crc register, the 1's line up
	uint32_t lineUpCheck = (1 << polynomialMS1B);
	// Iterate over each byte of the input
	for (int i = 0; i < length; i++)
	{
		uint32_t byte = buffer[start + i];
		// Iterate over each bit in the current byte
		for (int b = 7; b >= 0; b--)
		{
			// Shift crc 1 bit left
			crc <<= 1;
			// If bit b in the current byte is set
			if ((byte & (1 << b)) != 0)
			{
				// Add a set bit to the crc
				crc |= 0x01;
			}

			// The 1's line up in the crc
			if ((crc & lineUpCheck) != 0)
			{
				// Xor with the polynomial
				crc = xor(crc, polynomial);
			}
		}
	}
	// Save current crc value for the next block of data
	*initialization = crc;
	// All data processed, now we need to shift n more times for CRC-n because of the padding
	for (int i = 0; i < polynomialMS1B; i++)
	{
		// Same thing as above, but we don't have to add anything because we're always shifting in zeroes
		crc <<= 1;
		if ((crc & lineUpCheck) != 0)
		{
			crc = xor(crc, polynomial);
		}
	}
	return crc;
}

void crc_mode_c(char *txtFile)
{
	char buffer[BUFFER_SIZE];

	read_data(txtFile, buffer);

	printf("\nCRC-15 calculation progress:\n\n");

	// Print out each line of 64 bits of the unformatted text file and calculate the CRC and append to the end
	uint32_t crc_register = 0;
	for (int i = 0; i < 504; i += 64)
	{
		int length = 64;
		if (i + length > 504) 
		{ 
			length = 504 - i; 
		}

		char temp = buffer[i + length];
		buffer[i + length] = 0;
		uint32_t crc_value = crc(buffer, i, length, 0xA053, &crc_register);

		if (length == 56)
		{
			sprintf(buffer + i + length, "%08x", crc_value);
			temp = buffer[i + length];
		}
		printf("%s - %08x\n", buffer + i, crc_value);
		buffer[i + length] = temp;
	}

	printf("\nCRC-15 result : %s\n", buffer + 504);
}

void crc_mode_v(char *txtFile)
{
	char buffer[BUFFER_SIZE];

	read_data(txtFile, buffer);

	char crc_buffer[9];
	strcpy(crc_buffer, buffer + 504);

	printf("\nCRC-15 calculation progress:\n\n");

	// Print out each line of 64 bits of the unformatted text file and calculate the CRC and append to the end
	uint32_t crc_register = 0;
	for (int i = 0; i < 504; i += 64)
	{
		int length = 64;
		if (i + length > 504)
		{
			length = 504 - i;
		}

		char temp = buffer[i + length];
		buffer[i + length] = 0;
		uint32_t crc_value = crc(buffer, i, length, 0xA053, &crc_register);

		if (length == 56)
		{
			sprintf(buffer + i + length, "%08x", crc_value);
			temp = buffer[i + length];
		}
		printf("%s - %08x\n", buffer + i, crc_value);
		buffer[i + length] = temp;
	}

	if (strcmp(crc_buffer, buffer + 504) == 0)
	{
		printf("CRC 15 verification passed\n");
	}
	else
	{
		printf("CRC 15 verification failed\n");
	}
}