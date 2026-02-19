// Copyright Munteanu Eugen 315CAb 2022-2023
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// showing variable name in error message (defensive programming)
#define var_name(name) #name

struct image_data {
	char type[2]; // image type, e.g. P5

	int width; int height;

	int x1; int y1; // for cases: CROP, SELECT ALL,
	int x2; int y2; // SELECT <x1> <y1> <x2> <y2> etc.

	int max_color;
	int ***area;
	// area[0][i][j]    - grayscale image
	// area[0..2][i][j] - color image
};

int is_number(char x)
{
	// Check if x is a digit or not
	if ('0' <= x && x <= '9')
		return 1;

	return 0;
}

int skip_whitespaces(char x)
{
	// Check if x is a whitespace character
	// (condition used in binary file cases)
	if (x == '\n' || x == '\t' || x == '\r')
		return 1;

	return 0;
}

double clamp(double x, double min_value, double max_value)
{
	// restrict x value to be in the 'ascii' interval [0,255]
	if (x < min_value)
		x = min_value;
	else
		if (x > max_value)
			x = max_value;

	return x;
}

int aloc_triple_ptr(int ****ptr, int type, int lines, int elems)
{
	// triple int*** ptr dynamic allocation
	*ptr = (int ***)malloc(type * sizeof(int **));
	for (int k = 0; k < type; k++) {
		(*ptr)[k] = (int **)malloc(lines * sizeof(int *));
		if (!((*ptr)[k])) {
			fprintf(stderr, "Malloc for %s failed\n", var_name(*ptr));
			for (int i = 0; i < k; i++)
				free((*ptr)[k]);
			free(*ptr); *ptr = NULL;
			return 0;
		}
	}
	for (int k = 0; k < type; k++)
		for (int i = 0; i < lines; i++) {
			(*ptr)[k][i] = (int *)malloc(elems * sizeof(int));
			if (!((*ptr)[k][i])) {
				fprintf(stderr, "Malloc for %s failed\n", var_name(*ptr));
				for (int k_f = 0; k_f < type; k_f++) {
					for (int j = 0; j < i; j++)
						free((*ptr)[k_f][j]);
					free((*ptr)[k_f]);
				}
				free(*ptr); (*ptr) = NULL;
				return 0;
			}
		}
	return 1;
}

void free_image(struct image_data *image, int all)
{
	// free all allocated resources for image
	if ((*image).type[1] == '2' || (*image).type[1] == '5') {
		// grayscale image

		for (int i = 0; i < (*image).height; i++)
			if ((*image).area[0][i])
				free((*image).area[0][i]);
		if ((*image).area[0])
			free((*image).area[0]);
		if ((*image).area)
			free((*image).area);
		(*image).area = NULL;

	} else {
		// color image

		for (int k = 0; k < 3; k++) {
			for (int i = 0; i < (*image).height; i++)
				if ((*image).area[k][i])
					free((*image).area[k][i]);
			if ((*image).area[k])
				free((*image).area[k]);
		}
		if ((*image).area)
			free((*image).area);
		(*image).area = NULL;
	}

	// reinitialize variables to null if given, else
	// keep metadata for a possible new image
	if (all == 1) {
		(*image).type[0] = '\0'; (*image).type[1] = '\0';

		(*image).width = 0; (*image).height = 0;
		(*image).x1 = 0; (*image).y1 = 0;
		(*image).x2 = 0; (*image).y2 = 0;

		(*image).max_color = 0;
	}
}

void read_before_matrix(FILE **image_file, struct image_data *image)
{
	// read the input data from file, stopping at the beginning of the matrix

	// find the image type in the file (P2/P3/P5/P6)
	rewind(*image_file);
	for (int i = 0; i < 2; i++)
		(*image).type[i] = fgetc(*image_file);

	int binary = 0; // if file is binary or not

	// iterate through file character by character
	unsigned char chr = fgetc(*image_file);

	while (1) {
		// skip comments
		if (chr == '#') {
			char *comment; fscanf(*image_file, "%m[^\n]", &comment);
			fgetc(*image_file); // skip newline
			free(comment);
			chr = fgetc(*image_file); continue;
		}

		// if the file is not binary, skip whitespaces
		if (!(is_number(chr)) && !binary) {
			chr = fgetc(*image_file); continue;
		}

		// read width and height of the image
		if ((*image).width == 0 && (*image).height == 0) {
			// iterate through file character by character,
			// skipping whitespaces to get them properly

			// width
			fseek(*image_file, -1, SEEK_CUR);
			char *aux = (char *)calloc(10, sizeof(char));
			char num = fgetc(*image_file);
			int i = 0;
			while (is_number(num)) {
				aux[i++] = num; num = fgetc(*image_file);
			}
			(*image).width = atoi(aux); free(aux);

			// height
			while (!(is_number(num)))
				num = fgetc(*image_file);
			aux = (char *)calloc(10, sizeof(char));
			i = 0;
			while (is_number(num)) {
				aux[i++] = num; num = fgetc(*image_file);
			}
			(*image).height = atoi(aux); free(aux);

			chr = fgetc(*image_file); continue;
		}

		// read max value of a pixel
		if ((*image).max_color == 0) {
			char *aux = (char *)calloc(5, sizeof(char));
			int i = 0;
			while (is_number(chr)) {
				aux[i++] = chr; chr = fgetc(*image_file);
			}
			(*image).max_color = atoi(aux); free(aux);

			// now, we're at the beginning of the matrix, check
			// if the file is binary
			if ((*image).type[1] == '5' || (*image).type[1] == '6') {
				binary = 1;
				chr = fgetc(*image_file);
			}
			continue;
		}
		// in another function we will load the matrix in memory,
		// depending on the file and image type
		fseek(*image_file, -1, SEEK_CUR); return;
	}
}

void P2_case(FILE **image_file, struct image_data *image)
{
	// P2 case (text file, grayscale image)

	// free resources (if another image exists)
	if ((*image).area)
		free_image(&(*image), 1);

	// read input data until the beginning of the matrix
	read_before_matrix(&(*image_file), &(*image));

	unsigned char chr = fgetc(*image_file);
	// allocate memory for the matrix
	// convention -- grayscale: (*image).area[0][i][j]

	int m = (*image).width;
	int n = (*image).height;

	aloc_triple_ptr(&((*image).area), 1, n, m);

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			// next element in the matrix

			while (!(is_number(chr)))
				chr = fgetc(*image_file);
			int k = 0;
			char *aux = (char *)calloc(5, sizeof(char));
			while ((is_number(chr))) {
				aux[k++] = chr;
				chr = fgetc(*image_file);
			}
			(*image).area[0][i][j] = atoi(aux);

			free(aux);
		}
	}
}

void P3_case(FILE **image_file, struct image_data *image)
{
	// P3 case (text file, color image)

	// free resources (if another image exists)
	if ((*image).area)
		free_image(&(*image), 1);

	// read input data until the beginning of the matrix
	read_before_matrix(&(*image_file), &(*image));

	unsigned char chr = fgetc(*image_file);

	// allocate memory for the matrix
	// convention -- color: (*image).area[0..2][i][j], where
	// 0 - red; 1 - green; 2 - blue

	int m = (*image).width;
	int n = (*image).height;

	aloc_triple_ptr(&((*image).area), 3, n, m);

	for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
			for (int k = 0; k < 3; k++) {
				// next element in the matrix

				while (!(is_number(chr)))
					chr = fgetc(*image_file);
				char *aux = (char *)calloc(10, sizeof(char));
				int nr = 0;
				while ((is_number(chr))) {
					aux[nr++] = chr;
					chr = fgetc(*image_file);
				}
				(*image).area[k][i][j] = atoi(aux);

				free(aux);
			}
}

void P5_case(FILE **image_file, struct image_data *image)
{
	// P5 case (binary file, grayscale image)

	// free resources (if another image exists)
	if ((*image).area)
		free_image(&(*image), 1);

	// read input data until the beginning of the matrix
	read_before_matrix(&(*image_file), &(*image));

	unsigned char chr = fgetc(*image_file);

	// check if we're right before the first element in the image matrix
	if (chr == '\r' || chr == '\n')
		chr = fgetc(*image_file);

	int num_int = 0; // chr -> int transformation

	// allocate memory for the matrix
	// convention -- grayscale: (*image).area[0][i][j]

	int m = (*image).width;
	int n = (*image).height;

	aloc_triple_ptr(&((*image).area), 1, n, m);

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			// next element in the matrix

			num_int = (int)chr;
			(*image).area[0][i][j] = num_int;
			chr = fgetc(*image_file);
		}
	}
}

void P6_case(FILE **image_file, struct image_data *image)
{
	// P6 case (binary file, color image)

	// free resources (if another image exists)
	if ((*image).area)
		free_image(&(*image), 1);

	// read input data until the beginning of the matrix
	read_before_matrix(&(*image_file), &(*image));

	unsigned char chr = fgetc(*image_file);

	// check if we're right before the first element in the image matrix
	if (chr == '\r' || chr == '\n')
		chr = fgetc(*image_file);

	int num_int = 0; // chr -> int transformation

	// allocate memory for the matrix
	// convention -- color: (*image).area[0..2][i][j], where
	// 0 - red; 1 - green; 2 - blue

	int m = (*image).width;
	int n = (*image).height;

	aloc_triple_ptr(&((*image).area), 3, n, m);

	for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
			for (int k = 0; k < 3; k++) {
				// next element in the matrix

				num_int = (int)chr;
				(*image).area[k][i][j] = num_int;
				chr = fgetc(*image_file);
			}
}

void load_file(char **command, struct image_data *image)
{
	// LOAD <file> command

	// load in memory the file transmitted as parameter, if it exists; else,
	// free a possible loaded image

	FILE *image_file = fopen(*command + 5, "rt");
	if (!image_file) {
		if ((*image).area)
			free_image(&(*image), 1);
		printf("Failed to load %s\n", *command + 5);
		return;
	}

	// depending on the file type (P2/P3/P5/P6),
	// we will read the image matrix, element by element

	char word;
	word = fgetc(image_file); word = fgetc(image_file);

	switch (word) {
	case '2':
		P2_case(&image_file, &(*image)); break;
	case '3':
		P3_case(&image_file, &(*image)); break;
	case '5':
		P5_case(&image_file, &(*image)); break;
	case '6':
		P6_case(&image_file, &(*image)); break;
	}

	// in LOAD command case, the whole image is selected
	(*image).x1 = 0; (*image).y1 = 0;
	(*image).x2 = (*image).width; (*image).y2 = (*image).height;

	printf("Loaded %s\n", *command + 5);

	fclose(image_file);
}

int select_valid(char *token)
{
	// used for SELECT command, check if
	// the token is a valid integer (positive or negative)
	for (size_t i = 0; i < strlen(token); i++)
		if (!(('0' <= token[i] && token[i] <= '9') || token[i] == '-'))
			return 0;

	return 1;
}

void order_coords(int **coords)
{
	// used for SELECT command, check if
	// the coords need to be swapped, in order to have x1 <= x2 and y1 <= y2
	if ((*coords)[0] > (*coords)[2]) {
		(*coords)[0] = (*coords)[0] ^ (*coords)[2];
		(*coords)[2] = (*coords)[0] ^ (*coords)[2];
		(*coords)[0] = (*coords)[0] ^ (*coords)[2];
	}

	if ((*coords)[1] > (*coords)[3]) {
		(*coords)[1] = (*coords)[1] ^ (*coords)[3];
		(*coords)[3] = (*coords)[1] ^ (*coords)[3];
		(*coords)[1] = (*coords)[1] ^ (*coords)[3];
	}
}

int valid_coords(int **coords, struct image_data *image)
{
	// used for SELECT command, check if
	// the coordinates are valid
	for (int i = 0; i < 4; i++)
		if ((*coords)[i] < 0)
			return 0;

	if ((*coords)[2] > (*image).width || (*coords)[3] > (*image).height)
		return 0;

	if ((*coords)[0] == (*coords)[2] || (*coords)[1] == (*coords)[3])
		return 0;

	return 1;
}

void select_area(char **command, struct image_data *image)
{
	// SELECT <x1> <y1> <x2> <y2> command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	// allocate memory for coordinates
	// check if the given SELECT is valid
	char *all_coords = *command + 6;
	if (all_coords[0] != ' ') {
		printf("Invalid command\n");
		return;
	}
	char *token; token = strtok(all_coords + 1, " ");

	int *coords; coords = (int *)calloc(5, sizeof(int));

	// while reading the coords, check each of them to be valid
	for (int i = 0; i < 4; i++)
		if (token && select_valid(token)) {
			coords[i] = atoi(token);
			token = strtok(NULL, " ");
		} else {
			printf("Invalid command\n");
			free(coords);
			return;
		}
	if (token) {
		printf("Invalid command\n");
		free(coords);
		return;
	}

	order_coords(&coords);

	// check if the interval is valid and update image_data struct
	if (valid_coords(&coords, &(*image))) {
		(*image).x1 = coords[0]; (*image).y1 = coords[1];
		(*image).x2 = coords[2]; (*image).y2 = coords[3];

		printf("Selected %d %d %d %d\n", coords[0], coords[1],
			   coords[2], coords[3]);
		free(coords);

	} else {
		printf("Invalid set of coordinates\n");
		free(coords);
	}
}

void select_all(struct image_data *image)
{
	// SELECT ALL command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	// update coords
	(*image).x1 = 0; (*image).y1 = 0;
	(*image).x2 = (*image).width; (*image).y2 = (*image).height;

	printf("Selected ALL\n");
}

int histogram_valid(char *token, char letter)
{
	// check if the token is a valid number (unsigned int)
	for (size_t i = 0; i < strlen(token); i++)
		if (!('0' <= token[i] && token[i] <= '9'))
			return 0;

	// check if y is a power of 2 in the interval [2,256]
	if (letter == 'y') {
		int y = atoi(token);
		if (y < 2 || y > 256)
			return 0;

		while (y > 1) {
			if (y % 2 != 0)
				return 0;
			y /= 2;
		}
	}

	return 1;
}

void histogram_exec(struct image_data *image, int x, int y)
{
	// convention -- grayscale: (*image).area[0][i][j]

	// calculate frequency of each pixel in the image
	int *fr; fr = (int *)calloc(256, sizeof(int));
	if (!fr) {
		fprintf(stderr, "Calloc for %s failed\n", var_name(fr));
		return;
	}
	for (int i = 0; i < (*image).height; i++)
		for (int j = 0; j < (*image).width; j++)
			fr[(*image).area[0][i][j]]++;

	int fr_max = -1;

	// calculate, in another vector, frequency for the number of intervals (y);
	// thus we will have the frequency for (256 / y) groups of pixels

	int *fr2; fr2 = (int *)calloc(256, sizeof(int));
	if (!fr2) {
		fprintf(stderr, "Calloc for %s failed\n", var_name(fr2));
		free(fr);
		return;
	}

	int line = 0;
	for (int i = 0; i < 256; i++) {
		fr2[line / (256 / y)] += fr[i];
		if (fr_max < fr2[line / (256 / y)])
			fr_max = fr2[line / (256 / y)];
		line++;
	}

	// output the Y rows, applying the formula and rounding down (floor)
	double num_stars = 0;
	for (int i = 0; i < y; i++) {
		num_stars = (double)((1.0 * fr2[i] / fr_max) * x);
		num_stars = (int)floor(num_stars);

		// output current y row
		printf("%d\t|\t", (int)num_stars);
		for (int i = 0; i < (int)num_stars; i++)
			printf("*");
		printf("\n");
	}

	free(fr); free(fr2);
}

void histogram_image(char **command, struct image_data *image)
{
	// HISTOGRAM <x> <y> command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	char *parameter = *command + 9; // skip "HISTOGRAM"
	if (parameter[0] != ' ') {
		printf("Invalid command\n");
		return;
	}

	// read parameters and check for validity
	int x, y;
	char *token; token = strtok(parameter + 1, " ");
	if (!token || !histogram_valid(token, 'x')) {
		printf("Invalid command\n");
		return;
	}
	x = atoi(token); // no. of stars

	token = strtok(NULL, " ");
	if (!token || !histogram_valid(token, 'y')) {
		printf("Invalid command\n");
		return;
	}
	y = atoi(token); // no. of bins

	// another check if we have EXACTLY two parameters
	token = strtok(NULL, " ");
	if (token) {
		printf("Invalid command\n");
		return;
	}

	// grayscale only
	if ((*image).type[1] == '3' || (*image).type[1] == '6') {
		printf("Black and white image needed\n");
		return;
	}

	// Histogram creation and display
	histogram_exec(&(*image), x, y);
}

void equalize_image(struct image_data *image)
{
	// EQUALIZE command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	// grayscale only
	if ((*image).type[1] == '3' || (*image).type[1] == '6') {
		printf("Black and white image needed\n");
		return;
	}

	// calculate frequency of each pixel in the image
	int *fr; fr = (int *)calloc(256, sizeof(int));
	if (!fr) {
		fprintf(stderr, "Calloc for %s failed\n", var_name(*fr));
		return;
	}

	for (int i = 0; i < (*image).height; i++)
		for (int j = 0; j < (*image).width; j++)
			fr[(*image).area[0][i][j]]++;

	// using given formula, for each pixel we calculate a new value
	int area_value = (*image).width * (*image).height;
	int sum_h_i = 0;
	double new_pixel = 0;

	for (int i = 0; i < (*image).height; i++)
		for (int j = 0; j < (*image).width; j++) {
			int crt_pixel = (*image).area[0][i][j];
			sum_h_i = 0;

			// calculate sum for current pixel
			for (int k = 0; k <= crt_pixel; k++)
				sum_h_i += fr[k];

			// calculate and assign new value for current pixel
			new_pixel = (double)(255.0 * (1.0 / area_value) * sum_h_i);
			new_pixel = clamp(new_pixel, 0, 255);
			new_pixel = round(new_pixel);

			(*image).area[0][i][j] = (int)new_pixel;
		}

	free(fr);
	printf("Equalize done\n");
}

int rotate_valid(char *token)
{
	// for ROTATE command, check if
	// the token is a valid number (positive or negative)
	for (size_t i = 0; i < strlen(token); i++)
		if (!(('0' <= token[i] && token[i] <= '9') || token[i] == '-'))
			return 0;

	return 1;
}

void rotate_exec(struct image_data *image, int ****copy,
				 int ang_value, int type_matrix, int all_area)
{
	int width = (*image).x2 - (*image).x1;
	int height = (*image).y2 - (*image).y1;

	// depending on the angle value, assign new values for the
	// pixels/channels of the image loaded; iterate through
	// ***copy elements in a specific order, depending on the angle value

	// all_area = 1 -> whole image
	// all_area = 0 -> cropped area

	switch (ang_value) {
	case 90: case -270: {
		for (int k = 0; k < type_matrix; k++) {
			int pos_x1, pos_y1;
			if (all_area == 0) {
				pos_x1 = (*image).x1;
				pos_y1 = (*image).y1;
			} else {
				pos_x1 = 0;
				pos_y1 = 0;
			}

			for (int j = 0; j < width; j++) {
				for (int i = height - 1; i >= 0; i--) {
					(*image).area[k][pos_y1][pos_x1] = (*copy)[k][i][j];
					pos_x1++;
				}
				if (all_area == 0)
					pos_x1 = (*image).x1;
				else
					pos_x1 = 0;

				pos_y1++;
			}
		}
		break;
	}

	case -90: case 270: {
		for (int k = 0; k < type_matrix; k++) {
			int pos_x1, pos_y1;
			if (all_area == 0) {
				pos_x1 = (*image).x1;
				pos_y1 = (*image).y1;
			} else {
				pos_x1 = 0;
				pos_y1 = 0;
			}

			for (int j = width - 1; j >= 0; j--) {
				for (int i = 0; i < height; i++) {
					(*image).area[k][pos_y1][pos_x1] = (*copy)[k][i][j];
					pos_x1++;
				}
				if (all_area == 0)
					pos_x1 = (*image).x1;
				else
					pos_x1 = 0;

				pos_y1++;
			}
		}
		break;
	}
	}
}

void rotate_select(struct image_data *image, int ang_value)
{
	// ROTATE case - select a portion of the image to rotate it

	// currently loaded image type
	int type_matrix;
	if ((*image).type[1] == '2' || (*image).type[1] == '5')
		type_matrix = 1; // grayscale
	else
		type_matrix = 3; // color

	// depending of the image type, allocate memory and copy the
	// selected area of the image matrix in the variable ***copy

	int width = (*image).x2 - (*image).x1;
	int height = (*image).y2 - (*image).y1;

	int ***copy;
	aloc_triple_ptr(&copy, type_matrix, height, width);

	for (int k = 0; k < type_matrix; k++)
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				int sel_y = (*image).y1 + i;
				int sel_x = (*image).x1 + j;
				copy[k][i][j] = (*image).area[k][sel_y][sel_x];
			}

	// assign new values in the image matrix,
	// using the copy of the selected area
	rotate_exec(&(*image), &copy, ang_value, type_matrix, 0);

	// free auxiliary matrix
	for (int k = 0; k < type_matrix; k++) {
		for (int i = 0; i < height; i++)
			if (copy[k][i])
				free(copy[k][i]);
		if (copy[k])
			free(copy[k]);
	}
	free(copy);
}

void rotate_all(struct image_data *image, int ang_value)
{
	// ROTATE case - select the whole image to rotate it

	// currently loaded image type
	int type_matrix;
	if ((*image).type[1] == '2' || (*image).type[1] == '5')
		type_matrix = 1;
	else
		type_matrix = 3;

	// depending on the image type, allocate memory and copy the
	// whole image matrix in the variable ***copy
	int ***copy;
	aloc_triple_ptr(&copy, type_matrix, (*image).height, (*image).width);

	for (int k = 0; k < type_matrix; k++)
		for (int i = 0; i < (*image).height; i++)
			for (int j = 0; j < (*image).width; j++)
				copy[k][i][j] = (*image).area[k][i][j];

	// memory reallocation for the image, with changed dimensions
	// (-90/90 degrees rotation case)
	free_image(&(*image), 0);
	aloc_triple_ptr(&((*image).area), type_matrix,
					(*image).width, (*image).height);

	// assign new values in the image matrix
	rotate_exec(&(*image), &copy, ang_value, type_matrix, 1);

	// after rotation, swap dimensions and coords
	int aux;
	aux = (*image).width;
	(*image).width = (*image).height;
	(*image).height = aux;

	(*image).x1 = 0; (*image).y1 = 0;
	aux = (*image).x2;
	(*image).x2 = (*image).y2;
	(*image).y2 = aux;

	// free auxiliary matrix
	for (int k = 0; k < type_matrix; k++) {
		for (int i = 0; i < (*image).width; i++)
			if (copy[k][i])
				free(copy[k][i]);
		if (copy[k])
			free(copy[k]);
	}
	free(copy);
}

void rotate_area(char **command, struct image_data *image)
{
	// ROTATE <angle> command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	// check for valid angle
	char *angle = *command + 6;
	if (angle[0] != ' ') {
		printf("Invalid command\n");
		return;
	}

	char *token = strtok(angle + 1, " ");
	if (!token || !rotate_valid(token)) {
		printf("Invalid command\n");
		return;
	}
	int ang_value = atoi(token);

	// check if we have EXACTLY one parameter
	token = strtok(NULL, " ");
	if (token) {
		printf("Invalid command\n");
		return;
	}

	// check if the angle is valid for rotation (±90, ±180, ±270, ±360)
	if (ang_value < -360 || ang_value > 360 || ang_value % 90 != 0) {
		printf("Unsupported rotation angle\n");
		return;
	}

	// no rotation needed for 0/360 cases
	if (ang_value == -360 || ang_value == 0 || ang_value == 360) {
		printf("Rotated %d\n", ang_value);
		return;
	}

	// rotation cases: whole image (if...) or cropped area (else if...)
	if (((*image).x1 == 0 && (*image).x2 == (*image).width) &&
		((*image).y1 == 0 && (*image).y2 == (*image).height)) {
		if (ang_value != 180 && ang_value != -180) {
			rotate_all(&(*image), ang_value);
		} else {
			// for 180 degrees rotation, we rotate twice
			// of course, this is not the most efficient way, but it is
			// easier to implement
			rotate_all(&(*image), 90);
			rotate_all(&(*image), 90);
		}

	} else {
		if (((*image).x2 - (*image).x1) == ((*image).y2 - (*image).y1)) {
			if (ang_value != 180 && ang_value != -180) {
				rotate_select(&(*image), ang_value);
			} else {
				rotate_select(&(*image), 90);
				rotate_select(&(*image), 90);
			}

		} else {
			printf("The selection must be square\n");
			return;
		}
	}

	printf("Rotated %d\n", ang_value);
}

void crop_exec(struct image_data *image, int type_matrix)
{
	// make a copy of the image matrix, with the selected area (if exists);
	// else copy the whole image matrix
	int ***copy;
	int new_width = (*image).x2 - (*image).x1;
	int new_height = (*image).y2 - (*image).y1;

	aloc_triple_ptr(&copy, type_matrix, new_height, new_width);

	int crop_y = (*image).y1, crop_x = (*image).x1;
	for (int i = 0; i < new_height; i++)
		for (int j = 0; j < new_width; j++)
			for (int k = 0; k < type_matrix; k++)
				copy[k][i][j] = (*image).area[k][crop_y + i][crop_x + j];

	// after assigning the new values in the copy, free initial image matrix
	free_image(&(*image), 0);

	aloc_triple_ptr(&((*image).area), type_matrix, new_height, new_width);

	// assign the values from the copy of the selected area (or whole image)
	// in the image matrix of the struct (*image)
	for (int i = 0; i < new_height; i++)
		for (int j = 0; j < new_width; j++)
			for (int k = 0; k < type_matrix; k++)
				(*image).area[k][i][j] = copy[k][i][j];

	// free auxiliary matrix
	for (int k = 0; k < type_matrix; k++) {
		for (int i = 0; i < new_height; i++)
			free(copy[k][i]);
		free(copy[k]);
	}
	free(copy);

	// update image dimensions and coords
	(*image).x1 = 0; (*image).y1 = 0;
	(*image).width = new_width; (*image).x2 = new_width;
	(*image).height = new_height; (*image).y2 = new_height;
}

void crop_image(struct image_data *image)
{
	// CROP command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	// depending on the file type (P2/P5 or P3/P6),
	// call the appropriate function
	if ((*image).type[1] == '2' || ((*image).type[1] == '5'))
		crop_exec(&(*image), 1);
	else
		crop_exec(&(*image), 3);

	printf("Image cropped\n");
}

void apply_init(struct image_data *image, int *w, int *w_max,
				int *h, int *h_max)
{
	// for APPLY command, initialize coords for the selected area
	*w = (*image).x1; *w_max = (*image).x2;
	*h = (*image).y1; *h_max = (*image).y2;

	// margin pixels are not taken into account, only used for
	// calculating values of their neighbors
	if (*w == 0)
		(*w)++;
	if (*w_max == (*image).width)
		(*w_max)--;

	if (*h == 0)
		(*h)++;

	if (*h_max == (*image).height)
		(*h_max)--;
}

void apply_init_mat(double ***mat, char param)
{
	// Init 3x3 matrix to apply the parameter in the loaded image
	switch (param) {
	case 'E': {
		(*mat)[0][0] = -1; (*mat)[0][1] = -1; (*mat)[0][2] = -1;
		(*mat)[1][0] = -1; (*mat)[1][1] = 8; (*mat)[1][2] = -1;
		(*mat)[2][0] = -1; (*mat)[2][1] = -1; (*mat)[2][2] = -1;
		break;
	}

	case 'S': {
		(*mat)[0][0] = 0; (*mat)[0][1] = -1; (*mat)[0][2] = 0;
		(*mat)[1][0] = -1; (*mat)[1][1] = 5; (*mat)[1][2] = -1;
		(*mat)[2][0] = 0; (*mat)[2][1] = -1; (*mat)[2][2] = 0;
		break;
	}

	case 'B': {
		(*mat)[0][0] = 1; (*mat)[0][1] = 1; (*mat)[0][2] = 1;
		(*mat)[1][0] = 1; (*mat)[1][1] = 1; (*mat)[1][2] = 1;
		(*mat)[2][0] = 1; (*mat)[2][1] = 1; (*mat)[2][2] = 1;
		break;
	}

	case 'G': {
		(*mat)[0][0] = 1; (*mat)[0][1] = 2; (*mat)[0][2] = 1;
		(*mat)[1][0] = 2; (*mat)[1][1] = 4; (*mat)[1][2] = 2;
		(*mat)[2][0] = 1; (*mat)[2][1] = 2; (*mat)[2][2] = 1;
		break;
	}
	}
}

void apply_exec(struct image_data *image, char param)
{
	// check which pixels will be modified (margins not taken)
	int w, w_max, h, h_max;
	apply_init(&(*image), &w, &w_max, &h, &h_max);

	// allocate matrix for APPLY command
	double **mat; mat = (double **)malloc(3 * sizeof(double *));
	for (int i = 0; i < 3; i++)
		mat[i] = (double *)malloc(3 * sizeof(double));

	double sum = 0; // sum of current pixel (R/G/B)

	// for each pixel, copy the values obtained temporarily in a vector *v
	int total_size = (h_max - h + 1) * (w_max - w + 1) * 3;
	int *v; v = (int *)malloc(total_size * sizeof(int));
	if (!v) {
		fprintf(stderr, "Malloc for %s failed\n", var_name(*v));
		return;
	}

	int v_pos = 0;
	for (int i = h; i < h_max; i++)
		for (int j = w; j < w_max; j++)
			for (int k = 0; k < 3; k++) {
				// currently at (*image).area[k][h][w]
				// for each channel of each pixel, calculate the sum
				// corresponding to the parameter given in STDIN

				// init corresponding matrix for the APPLY type
				apply_init_mat(&mat, param);

				mat[0][0] *= (double)((*image).area[k][i - 1][j - 1]);
				mat[0][1] *= (double)((*image).area[k][i - 1][j]);
				mat[0][2] *= (double)((*image).area[k][i - 1][j + 1]);
				mat[1][0] *= (double)((*image).area[k][i][j - 1]);
				mat[1][1] *= (double)((*image).area[k][i][j]);
				mat[1][2] *= (double)((*image).area[k][i][j + 1]);
				mat[2][0] *= (double)((*image).area[k][i + 1][j - 1]);
				mat[2][1] *= (double)((*image).area[k][i + 1][j]);
				mat[2][2] *= (double)((*image).area[k][i + 1][j + 1]);

				// calculate sum
				sum = 0;
				for (int l = 0; l < 3; l++)
					for (int c = 0; c < 3; c++)
						sum += mat[l][c];

				// using the formula, modify the sum where needed
				if (param == 'B')
					sum = (double)(sum / 9);
				else if (param == 'G')
					sum = (double)(sum / 16);

				// apply clump(), round and add the result in auxiliary vector
				sum = clamp(sum, 0, 255); sum = round(sum);
				v[v_pos++] = (int)sum;
			}

	// next, copy the new values obtained
	v_pos = 0;
	for (int i = h; i < h_max; i++)
		for (int j = w; j < w_max; j++)
			for (int k = 0; k < 3; k++)
				(*image).area[k][i][j] = v[v_pos++];

	// free resources
	free(v);
	for (int i = 0; i < 3; i++)
		free(mat[i]);
	free(mat);
}

void apply_area(char **command, struct image_data *image)
{
	// APPLY <PARAMETER> command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	// apply the given effect, if it is valid

	char *parameter = *command + 5; // skip "APPLY"
	if (parameter[0] != ' ') {
		printf("Invalid command\n");
		return;
	}

	// check for parameter existence
	char *token; token = strtok(parameter, " ");
	if (!token) {
		printf("Invalid command\n");
		return;
	}

	// color only
	if ((*image).type[1] == '2' || (*image).type[1] == '5') {
		printf("Easy, Charlie Chaplin\n");
		return;
	}

	// call the corresponding function
	if (!strcmp(token, "EDGE")) {
		apply_exec(&(*image), 'E');
		printf("APPLY %s done\n", token);
		return;
	}

	if (!strcmp(token, "SHARPEN")) {
		apply_exec(&(*image), 'S');
		printf("APPLY %s done\n", token);
		return;
	}

	if (!strcmp(token, "BLUR")) {
		apply_exec(&(*image), 'B');
		printf("APPLY %s done\n", token);
		return;
	}

	if (!strcmp(token, "GAUSSIAN_BLUR")) {
		apply_exec(&(*image), 'G');
		printf("APPLY %s done\n", token);
		return;
	}

	printf("APPLY parameter invalid\n");
}

void write_before_matrix(FILE **image_file, struct image_data *image, int *save)
{
	// write in the file the input data,
	// stopping at the beginning of the matrix
	fputc((*image).type[0], *image_file);

	// depending on the file type and save location  (0 - binary, 1 - text),
	// update *save variable
	if ((*image).type[1] == '2' || ((*image).type[1] == '5')) {
		// grayscale image

		if (*save == 1) {
			*save = 2;
			fputc('2', *image_file);
		} else {
			*save = 5;
			fputc('5', *image_file);
		}

	} else {
		// color image

		if (*save == 1) {
			*save = 3;
			fputc('3', *image_file);
		} else {
			*save = 6;
			fputc('6', *image_file);
		}
	}
	fputc('\n', *image_file);

	// write next set of input data, without the matrix
	fprintf(*image_file, "%d %d\n", (*image).width, (*image).height);
	fprintf(*image_file, "%d\n", (*image).max_color);
}

void save_file(char **command, struct image_data *image)
{
	// SAVE <file> [ascii] command

	// check for existing image
	if (!(*image).area) {
		printf("No image loaded\n");
		return;
	}

	char *pos = *command + 5;
	int i = 0, mem = 0;

	// save filename in *image_name
	char *image_name;
	if (pos[i] != ' ' && pos[i] != '\0') {
		image_name = (char *)calloc(128, sizeof(char));
		if (!image_name) {
			fprintf(stderr, "Calloc for %s failed\n", var_name(image_name));
			return;
		}
		image_name[mem++] = pos[i++];
		while (pos[i] != ' ' && pos[i] != '\0')
			image_name[mem++] = pos[i++];
	}

	// check if the optional 'ascii' was given

	int save; // 0 - binary, 1 - text
	FILE *image_file;
	if (!strcmp(pos, image_name)) {
		image_file = fopen(image_name, "wb");
		save = 0;
	} else {
		image_file = fopen(image_name, "wt");
		save = 1;
	}

	// depending on the file type (P2/P3/P5/P6), write the data
	write_before_matrix(&image_file, &(*image), &save);
	switch (save) {
	case 2: {
		// P2 - text file, grayscale image
		for (int i = 0; i < (*image).height; i++) {
			for (int j = 0; j < (*image).width; j++)
				fprintf(image_file, "%d ", (*image).area[0][i][j]);
			fprintf(image_file, "\n");
		}
		break;
	}
	case 3: {
		// P3 - text file, color image
		for (int i = 0; i < (*image).height; i++) {
			for (int j = 0; j < (*image).width; j++)
				for (int k = 0; k < 3; k++)
					fprintf(image_file, "%d ", (*image).area[k][i][j]);
			fprintf(image_file, "\n");
		}
		break;
	}
	case 5: {
		// P5 - grayscale image, binary file
		for (int i = 0; i < (*image).height; i++)
			for (int j = 0; j < (*image).width; j++)
				fprintf(image_file, "%c", (char)(*image).area[0][i][j]);
		break;
	}
	case 6: {
		// P6 - color image, binary file
		for (int i = 0; i < (*image).height; i++)
			for (int j = 0; j < (*image).width; j++)
				for (int k = 0; k < 3; k++)
					fprintf(image_file, "%c", (char)(*image).area[k][i][j]);
		break;
	}
	}
	printf("Saved %s\n", image_name);
	fclose(image_file); free(image_name);
}

char command_selection(char *command, struct image_data image)
{
	// all commands will be shortened for switch cases (if they are valid)
	char *valid;

	char command_letter = '-';

	valid = strstr(command, "LOAD ");
	if (valid && !strcmp(command, valid))
		command_letter = 'L';

	valid = strstr(command, "SELECT ");
	if (valid && !strcmp(command, valid))
		command_letter = 's';

	valid = strstr(command, "SELECT ALL");
	if (valid && !strcmp(command, valid))
		command_letter = 'S';

	valid = strstr(command, "HISTOGRAM");
	if (valid && !strcmp(command, valid))
		command_letter = 'H';

	valid = strstr(command, "EQUALIZE");
	if (valid && !strcmp(command, valid))
		command_letter = 'E';

	valid = strstr(command, "ROTATE");
	if (valid && !strcmp(command, valid))
		command_letter = 'R';

	valid = strstr(command, "CROP");
	if (valid && !strcmp(command, valid))
		command_letter = 'C';

	valid = strstr(command, "APPLY");
	if (valid && !strcmp(command, valid))
		command_letter = 'A';

	valid = strstr(command, "SAVE ");
	if (valid && !strcmp(command, valid))
		command_letter = '$';

	valid = strstr(command, "EXIT");
	if (valid && !strcmp(command, valid)) {
		if (!(image.area))
			command_letter = '0';
		else
			command_letter = '1';
	}

	return command_letter;
}

int main(void)
{
	// input commands will be stored in *command
	char *command;

	// init image struct
	struct image_data image = {0};

	// Execute commands until we reach the EXIT case,
	// with a loaded image
	int run = 1;
	while (run) {
		scanf("%m[^\n]", &command); getchar();
		if (!command)
			break;

		// execute corresponding command
		char cmd = command_selection(command, image);

		switch (cmd) {
		case 'L': {
			load_file(&command, &image); break;
		}
		case 's': {
			select_area(&command, &image); break;
		}
		case 'S': {
			select_all(&image); break;
		}
		case 'H': {
			histogram_image(&command, &image); break;
		}
		case 'E': {
			equalize_image(&image); break;
		}
		case 'R': {
			rotate_area(&command, &image); break;
		}
		case 'C': {
			crop_image(&image); break;
		}
		case 'A': {
			apply_area(&command, &image); break;
		}
		case '$': {
			save_file(&command, &image); break;
		}
		case '0': {
			printf("No image loaded\n"); break;
		}
		case '1': {
			run = 0; break;
		}

		default: {
			printf("Invalid command\n"); break;
		}
		}

		if (command && cmd != '1')
			free(command);
	}

	// free resources
	if (command)
		free(command);
	if (image.area)
		free_image(&image, 1);

	return 0;
}
