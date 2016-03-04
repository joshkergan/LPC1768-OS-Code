int strcmp(char *s1, char *s2) {
	while (*s1 == *s2 && *s1 != '\0') {
		s1++;
		s2++;
	}
	
	if (*s1 == '\0' && *s2 == '\0')
		return 0;
	
	return (*s1 < *s2) ? -1 : 1;
}

void strcpy(char *src, char *dest) {
	if (!src || !dest)
		return;
	
	while (*src != '\0') {
		*(dest++) = *(src++);
	}
	
	*dest = '\0';
}

// Check if pre is a prefix of test
int is_prefix(char *pre, char *test) {
	while (*pre == *test && *pre != '\0') {
		pre++;
		test++;
	}
	
	return *pre == '\0';
}

int is_digit(char c) {
	return '0' <= c && c <= '9';
}
