
#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif


enum
{
    ParseBadParameter=1000,
    ParseBadValue,
    ParseTooMany,
    ParseNegativeIncrement,
    ParsePositiveIncrement,

    ParseMinimumDecimal,
    ParseMaximumDecimal,
    ParseMinimumHex,
    ParseMaximumHex,
    ParseMinimumDouble,

    ParseMaximumDouble,
    ParseError,
    ParseHelp,
    ParseHelpStart,
    ParseHelpEnd,

    ParseMinimumUnsigned,
    ParseMaximumUnsigned,
    ParseMinimumMac,
    ParseMaximumMac,
	ParseHelpSynopsisStart,

	ParseHelpSynopsisEnd,
	ParseHelpParametersStart,
	ParseHelpParametersEnd,
	ParseHelpDescriptionStart,
	ParseHelpDescriptionEnd,

	ParseHelpUnknown,
	ParseBadCommand,
	ParseBadArrayIndex,
	ParseArrayIndexBound,
	ParseArrayIndexBound2,

	ParseArrayIndexBound3,

    ParseCenterFrequencyUsed,
};

#define ParseBadParameterFormat "Unknown parameter \"%s\"."
#define ParseBadValueFormat "Bad value \"%s\" for parameter \"%s\"."
#define ParseTooManyFormat "Too many values for parameter \"%s\". Maximum is %d."
#define ParseNegativeIncrementFormat "End value must be smaller than start value for parameter \"%s\"."
#define ParsePositiveIncrementFormat "End value must be larger than start value for parameter \"%s\"."
#define ParseMinimumDecimalFormat "Value %d is smaller than the minimum value of %d for parameter \"%s\"."
#define ParseMaximumDecimalFormat "Value %d is greater than the maximum value of %d for parameter \"%s\"."
#define ParseMinimumHexFormat "Value 0x%x is smaller than the minimum value of 0x%x for parameter \"%s\"."
#define ParseMaximumHexFormat "Value 0x%x is greater than the maximum value of 0x%x for parameter \"%s\"."
#define ParseMinimumDoubleFormat "Value %lg is smaller than the minimum value of %lg for parameter \"%s\"."
#define ParseMaximumDoubleFormat "Value %lg is greater than the maximum value of %lg for parameter \"%s\"."
#define ParseErrorFormat "Command parsing error. Command not started."
#define ParseHelpFormat "%s"
#define ParseHelpStartFormat "Help start."
#define ParseHelpEndFormat "Help end."
#define ParseMinimumUnsignedFormat "Value %u is smaller than the minimum value of %u for parameter \"%s\"."
#define ParseMaximumUnsignedFormat "Value %u is greater than the maximum value of %u for parameter \"%s\"."
#define ParseMinimumMacFormat "Value %02x:%02x:%02x:%02x:%02x:%02x is smaller than the minimum value of %02x:%02x:%02x:%02x:%02x:%02x for parameter \"%s\"."
#define ParseMaximumMacFormat "Value %02x:%02x:%02x:%02x:%02x:%02x is greater than the maximum value of %02x:%02x:%02x:%02x:%02x:%02x for parameter \"%s\"."
#define ParseHelpSynopsisStartFormat "Synopsis:"
#define ParseHelpSynopsisEndFormat ""
#define ParseHelpParametersStartFormat "Parameters:"
#define ParseHelpParametersEndFormat ""
#define ParseHelpDescriptionStartFormat "Description:"
#define ParseHelpDescriptionEndFormat ""
#define ParseHelpUnknownFormat "I don't know."
#define ParseBadCommandFormat "Unknown command \"%s\"."
#define ParseBadArrayIndexFormat "Bad array index \"%s\"."
#define ParseArrayIndexBoundFormat "Array index [%d] is less than zero or greater than maximum [%d]."
#define ParseArrayIndexBound2Format "Array index [%d, %d] is less than zero or greater than maximum [%d, %d]."
#define ParseArrayIndexBound3Format "Array index [%d, %d, %d] is less than zero or greater than maximum [%d, %d, %d]."


#define ParseBadParameterFormatInput "Unknown parameter \"%[^\"]\"."
#define ParseBadValueFormatInput "Bad value \"%[^\"]\" for parameter \"%[^\"]\"."
#define ParseTooManyFormatInput "Too many values for parameter \"%[^\"]\". Maximum is %d."
#define ParseNegativeIncrementFormatInput "End value must be smaller than start value for parameter \"%[^\"]\"."
#define ParsePositiveIncrementFormatInput "End value must be larger than start value for parameter \"%[^\"]\"."
#define ParseMinimumDecimalFormatInput "Value %d is smaller than the minimum value of %d for parameter \"%[^\"]\"."
#define ParseMaximumDecimalFormatInput "Value %d is greater than the maximum value of %d for parameter \"%[^\"]\"."
#define ParseMinimumHexFormatInput "Value 0x%x is smaller than the minimum value of 0x%x for parameter \"%[^\"]\"."
#define ParseMaximumHexFormatInput "Value 0x%x is greater than the maximum value of 0x%x for parameter \"%[^\"]\"."
#define ParseMinimumDoubleFormatInput "Value %lg is smaller than the minimum value of %lg for parameter \"%[^\"]\"."
#define ParseMaximumDoubleFormatInput "Value %lg is greater than the maximum value of %lg for parameter \"%[^\"]\"."
#define ParseErrorFormatInput ParseErrorFormat
#define ParseHelpFormatInput "%[^\n]"
#define ParseHelpStartFormatInput ParseHelpStartFormat
#define ParseHelpEndFormatInput ParseHelpEndFormat
#define ParseMinimumUnsignedFormatInput "Value %u is smaller than the minimum value of %u for parameter \"%[^\"]\"."
#define ParseMaximumUnsignedFormatInput "Value %u is greater than the maximum value of %u for parameter \"%[^\"]\"."
#define ParseMinimumMacFormatInput "Value %02x:%02x:%02x:%02x:%02x:%02x is smaller than the minimum value of %02x:%02x:%02x:%02x:%02x:%02x for parameter \"%[^\"]\"."
#define ParseMaximumMacFormatInput "Value %02x:%02x:%02x:%02x:%02x:%02x is greater than the maximum value of %02x:%02x:%02x:%02x:%02x:%02x for parameter \"%[^\"]\"."
#define ParseHelpSynopsisStartFormatInput ParseHelpSynopsisStartFormat
#define ParseHelpSynopsisEndFormatInput ParseHelpSynopsisEndFormat
#define ParseHelpParametersStartFormatInput ParseHelpParametersStartFormat
#define ParseHelpParametersEndFormatInput ParseHelpParametersEndFormat
#define ParseHelpDescriptionStartFormatInput ParseHelpDescriptionStartFormat
#define ParseHelpDescriptionEndFormatInput ParseHelpDescriptionEndFormat
#define ParseHelpUnknownFormatInput ParseHelpUnknownFormat
#define ParseBadPCommandFormatInput "Unknown command \"%[^\"]\"."
#define ParseBadArrayIndexFormatInput "Bad array index \"%[^\"\"."
#define ParseArrayIndexBoundFormatInput ParseArrayIndexBoundFormat
#define ParseArrayIndexBound2FormatInput ParseArrayIndexBound2Format
#define ParseArrayIndexBound3FormatInput ParseArrayIndexBound3Format
#define ParseCenterFrequencyUsedFormat "Center frequency %d is already defined."
