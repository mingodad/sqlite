static const unsigned char AccentedChars[] = 	"ÇçÑñÀÁÂÃÄÅàáâãäåÈÉÊËèéêëÌÍÎÏìíîïÒÓÔÕÖØòóôõöøÙÚÛÜùúûüÝÿý";
static const unsigned char deAccentedChars[] =  "CcNnAAAAAAaaaaaaEEEEeeeeIIIIiiiiOOOOOOooooooUUUUuuuuYyy";
static const unsigned char lowerDeacccentedChars[] =  "ccnnaaaaaaaaaaaaeeeeeeeeiiiiiiiioooooooooooouuuuuuuuyyy";

static const unsigned char upperCaseAccentedChars[] = "ÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖØÙÚÛÜÝÑÇ";
static const unsigned char lowerCaseAccentedChars[] = "àáâãäåèéêëìíîïòóôõöøùúûüýñç";

static const unsigned char upperCaseChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXZY";
static const unsigned char lowerCaseChars[] = "abcdefghijklmnopqrstuvwxzy";

void initChar256(unsigned char *dest){
	int i;
	for(i=0; i<256;i++) *(dest+i) = i;
}

void clearChar256(unsigned char *dest){
	int i;
	for(i=0; i<256;i++) *(dest+i) = 0;
}

int main(void){
	unsigned char char256[256];
	int i, pos;
	char withComma[] = "%d,";
	char withoutComma[] = "%d";

	/*
		sizeof()-1 to left out the zero terminator
	*/
	//Deaccented
	initChar256(char256);

	for(i=0; i < sizeof(deAccentedChars)-1; i++){
		pos = AccentedChars[i*2+1];
		char256[pos] = deAccentedChars[i];
	}

	printf("\nconst unsigned char deAccentSubSetLatinUtf8MapTable[] = {");
	for(i=0; i < sizeof(char256); i++){
		if((i % 16) == 0) printf("//%d\n", i);
		printf( (i == 255 ? withoutComma : withComma) , char256[i]);
	}
	printf("\n};\n");

	//toLowerDeaccent
	initChar256(char256);

	for(i=0; i < sizeof(deAccentedChars)-1; i++){
		pos = AccentedChars[i*2+1];
		char256[pos] = lowerDeacccentedChars[i];
	}

	for(i=0; i < sizeof(upperCaseChars)-1;i++){
		pos = upperCaseChars[i];
		char256[pos] = lowerCaseChars[i];
	}

	printf("\nconst unsigned char toLowerDeaccentSubSetLatinUtf8MapTable[] = {");
	for(i=0; i < sizeof(char256); i++){
		if((i % 16) == 0) printf("//%d\n", i);
		printf( (i == 255 ? withoutComma : withComma) , char256[i]);
	}
	printf("\n};\n");

	//toLower
	initChar256(char256);

	for(i=0; i < sizeof(upperCaseAccentedChars)-1;){
		pos = upperCaseAccentedChars[i+1];
		char256[pos] = lowerCaseAccentedChars[i+1];
		i+=2;
	}

	for(i=0; i < sizeof(upperCaseChars)-1;i++){
		pos = upperCaseChars[i];
		char256[pos] = lowerCaseChars[i];
	}

	printf("\nconst unsigned char toLowerSubSetLatinUtf8MapTable[] = {");
	for(i=0; i < sizeof(char256); i++){
		if((i % 16) == 0) printf("//%d\n", i);
		printf( (i == 255 ? withoutComma : withComma) , char256[i]);
	}
	printf("\n};\n");

	//isLower
	clearChar256(char256);

	for(i=0; i < sizeof(lowerCaseAccentedChars)-1;){
		pos = lowerCaseAccentedChars[i+1];
		char256[pos] = lowerCaseAccentedChars[i+1];
		i+=2;
	}

	for(i=0; i < sizeof(lowerCaseChars)-1;i++){
		pos = lowerCaseChars[i];
		char256[pos] = lowerCaseChars[i];
	}

	printf("\nconst unsigned char isLowerSubSetLatinUtf8MapTable[] = {");
	for(i=0; i < sizeof(char256); i++){
		if((i % 16) == 0) printf("//%d\n", i);
		printf( (i == 255 ? withoutComma : withComma) , char256[i]);
	}
	printf("\n};\n");

	//toUpper
	initChar256(char256);

	for(i=0; i < sizeof(lowerCaseAccentedChars)-1;){
		pos = lowerCaseAccentedChars[i+1];
		char256[pos] = upperCaseAccentedChars[i+1];
		i+=2;
	}

	for(i=0; i < sizeof(lowerCaseChars)-1;i++){
		pos = lowerCaseChars[i];
		char256[pos] = upperCaseChars[i];
	}

	printf("\nconst unsigned char toUpperSubSetLatinUtf8MapTable[] = {");
	for(i=0; i < sizeof(char256); i++){
		if((i % 16) == 0) printf("//%d\n", i);
		printf( (i == 255 ? withoutComma : withComma) , char256[i]);
	}
	printf("\n};\n");

	//isUpper
	clearChar256(char256);

	for(i=0; i < sizeof(upperCaseAccentedChars)-1;){
		pos = upperCaseAccentedChars[i+1];
		char256[pos] = upperCaseAccentedChars[i+1];
		i+=2;
	}

	for(i=0; i < sizeof(upperCaseChars)-1;i++){
		pos = upperCaseChars[i];
		char256[pos] = upperCaseChars[i];
	}

	printf("\nconst unsigned char isUpperSubSetLatinUtf8MapTable[] = {");
	for(i=0; i < sizeof(char256); i++){
		if((i % 16) == 0) printf("//%d\n", i);
		printf( (i == 255 ? withoutComma : withComma) , char256[i]);
	}
	printf("\n};\n");


	//for(i=0; i < sizeof(AccentedChars); i++){
	//	if((i % 14) == 0) printf("\n");
	//	printf( (i == 255 ? withoutComma : withComma) , AccentedChars[i]);
	//}

	return 0;
}
