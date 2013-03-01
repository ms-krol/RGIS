/******************************************************************************

GHAAS Database library V2.1
Global Hydrologic Archive and Analysis System
Copyright 1994-2012, UNH - CCNY/CUNY

DBExpASCII.C

balazs.fekete@unh.edu

*******************************************************************************/

#include <DB.H>
#include <DBif.H>

#define BLOCKSIZE 128

static char *_DBGetLine(char *buffer, int *size, FILE *inFile)
	{
	char *ptr;
	int cont = false;

	if (buffer == (char *) NULL)
		{
		if ((buffer = (char *) malloc (BLOCKSIZE * sizeof (char))) == (char *) NULL)
			{
			CMmsgPrint (CMmsgSysError, "Memory Allocation Error in: %s %d",__FILE__,__LINE__);
			*size = 0;
			return ((char *) NULL);
			}
		else	*size = BLOCKSIZE;
		}

	ptr = buffer;
	do
		{
		if (fgets (ptr,*size - (ptr - buffer),inFile) == NULL)
			{ free (buffer); *size = 0; return ((char *) NULL); }
		if (((int) strlen (buffer) == *size - 1) && (buffer [strlen (buffer) - 1] != '\n'))
			{
			if ((buffer = (char *) realloc (buffer,(*size + BLOCKSIZE) * sizeof (char))) == (char *) NULL)
				{
				CMmsgPrint (CMmsgSysError, "Memory Allocation Error in: %s %d",__FILE__,__LINE__);
				*size = 0;
				return ((char *) NULL);
				}
			else *size += BLOCKSIZE;
			ptr = buffer + strlen (buffer);
			cont = true;
			}
		else cont = false;
		} while (cont);

	if (buffer [strlen (buffer) - 1] == '\n') buffer [strlen (buffer) - 1] = '\0';
	return (buffer);
	}


DBInt DBImportASCIITable (DBObjTable *table, const char *fileName)
	{
	DBInt ret;
	FILE *fp;
	if ((fp = fopen (fileName,"r")) == (FILE *) NULL)
		{ CMmsgPrint (CMmsgSysError, "File Opening Error in: %s %d",__FILE__,__LINE__); return (DBFault); }
	ret = DBImportASCIITable (table,fp);
	fclose (fp);
	return (ret);	
	}

class _DBImportASCIIRecord
	{
	private:
		char *Buffer, **FieldARRAY;
		int   FieldNumVAR;
	public:
	_DBImportASCIIRecord (const char *buffer)
		{
		int ret = DBFault, i, fieldID = 0, bufferLen;

		Buffer      = (char *) NULL;
		FieldARRAY  = (char **) NULL;
		FieldNumVAR = 1;
		bufferLen = strlen (buffer);
		if ((Buffer = (char *) malloc (bufferLen + 1)) != (char *) NULL)
			{
			strcpy (Buffer,buffer);

			for (i = 0;i < bufferLen; ++i) if (Buffer [i] == '\t') FieldNumVAR++; 
			if ((FieldARRAY = (char **) calloc (FieldNumVAR,sizeof (char *))) == (char **) NULL)
				{ CMmsgPrint (CMmsgSysError, "Memory Allocation Error in: %s %d",__FILE__,__LINE__); goto Stop; }
			FieldARRAY [fieldID++] = Buffer [0] == '"' ? Buffer + 1 : Buffer;
			for (i = 0;i < bufferLen; ++i)
				if (Buffer [i] == '\t')
					{
					Buffer [i] = '\0';
					if ((i > 1) && (Buffer [i - 1] == '"' )) Buffer [i - 1] = '\0';
					FieldARRAY [fieldID++] = Buffer [i + 1] == '"' ? Buffer + i + 2 : Buffer + i + 1;
					}
			if (Buffer [i - 1] == '"' ) Buffer [i - 1] = '\0';
			}
		ret = DBSuccess;
		Stop:
		if (ret == DBFault)
			{
			if (Buffer     != (char *)  NULL) free (Buffer);
			if (FieldARRAY != (char **) NULL) free (FieldARRAY);
			Buffer      = (char *) NULL;
			FieldARRAY  = (char **) NULL;
			FieldNumVAR = 0;
			}
		}
	int FieldNum () const { return (FieldNumVAR); }
	const char *Field (int fieldID) const { if (fieldID >= FieldNumVAR) return ((char *) NULL); else return (FieldARRAY [fieldID]); }
	~_DBImportASCIIRecord ()
		{
		if (Buffer     != (char *)  NULL) free (Buffer);
		if (FieldARRAY != (char **) NULL) free (FieldARRAY);
		}
	};

class _DBImportASCIIHeaderField
	{
	private:
		int TypeVAR;
		int LengthVAR;
		int DecimalsVAR;
	public:
		void Type     (int type)     { TypeVAR = type; }
		int  Type     () const { return (TypeVAR); }
		void Length   (int length)   { LengthVAR = length; }
		int  Length   () const { return (LengthVAR); }
		void Decimals (int decimals) { DecimalsVAR = decimals; }
		int  Decimals () const { return (DecimalsVAR); }
	};
class _DBImportASCIIHeader : public _DBImportASCIIRecord
	{
	private:
		_DBImportASCIIHeaderField *FieldsPTR;
	public:
		_DBImportASCIIHeader (const char *buffer) : _DBImportASCIIRecord (buffer)
			{
			if ((FieldsPTR = (_DBImportASCIIHeaderField *) calloc (FieldNum (),sizeof (_DBImportASCIIHeaderField))) == (_DBImportASCIIHeaderField *) NULL)
				CMmsgPrint (CMmsgSysError, "Memory Allocation Error in: %s %d",__FILE__,__LINE__);
			}
		~_DBImportASCIIHeader ()
			{
			if (FieldsPTR != (_DBImportASCIIHeaderField *) NULL) free (FieldsPTR);
			}
		int Ready () const { return (FieldsPTR != (_DBImportASCIIHeaderField *) NULL); }
		void Type     (int field, int type)     { FieldsPTR [field].Type (type); }
		int  Type     (int field) const { return (FieldsPTR [field].Type ()); }
		void Length   (int field, int length)   { FieldsPTR [field].Length (length); }
		int  Length   (int field) const { return (FieldsPTR [field].Length ()); }
		void Decimals (int field, int decimals) { FieldsPTR [field].Decimals (decimals); }
		int  Decimals (int field) const { return (FieldsPTR [field].Decimals ()); }
	};

DBInt DBImportASCIITable (DBObjTable *table, FILE *fp)

	{
	DBObjRecord *recordPTR;
	DBObjTableField *fieldFLD;
	char *buffer = (char *) NULL, format [20], recordName [32];
	const char *fieldSTR;
	int   bufferSize = 0, nDecimals, ret = DBSuccess;
	int   recordNum  = 0, recordID, fieldID, i;
	int    intVal;
	double floatVal;
	_DBImportASCIIHeader *header;
	_DBImportASCIIRecord **records = (_DBImportASCIIRecord **) NULL;
	
	if ((buffer = _DBGetLine (buffer, &bufferSize, fp)) == (char *) NULL)
		{ CMmsgPrint (CMmsgSysError, "File Reading Error in: %s %d",__FILE__,__LINE__); return (DBFault); }
	header = new _DBImportASCIIHeader (buffer);

	if (header->Ready ())
		{
		while ((buffer = _DBGetLine (buffer, &bufferSize, fp)) != (char *) NULL)
			{
			if ((records = (_DBImportASCIIRecord **) realloc (records, sizeof (_DBImportASCIIRecord *) * (recordNum + 1))) == (_DBImportASCIIRecord **) NULL)
				{
				CMmsgPrint (CMmsgSysError, "Memory Allocation Error in: %s %d",__FILE__,__LINE__);
				delete header;
				ret = DBFault;
				goto Stop;
				}
			records [recordNum] = new _DBImportASCIIRecord (buffer);
			recordNum++;
			}
		for (fieldID = 0; fieldID < header->FieldNum(); fieldID++)
			{
			header->Type (fieldID,DBVariableInt);
			header->Length   (fieldID, 0);
			header->Decimals (fieldID, 0);
			for (recordID = 0;recordID < recordNum; recordID++)
				{
				if ((fieldSTR = records[recordID]->Field(fieldID)) == (char *) NULL) continue;
				if (header->Length(fieldID) < (int) strlen (records[recordID]->Field(fieldID)))
					header->Length(fieldID, strlen (records[recordID]->Field(fieldID)));
				if (header->Type (fieldID) == DBVariableString) continue;
				nDecimals = 0;
				for (i = 0;i < (int) strlen (fieldSTR); i++)
					{
					if ((fieldSTR [i] >= '0') && (fieldSTR [i] <= '9')) continue;
					if (fieldSTR [i] == '.')
						{
						if ((header->Type (fieldID) == DBVariableFloat) && (nDecimals != 0))
							header->Type (fieldID,DBVariableString);
						else
							{
							header->Type (fieldID,DBVariableFloat);
							nDecimals = strlen (fieldSTR + i + 1);
							if (nDecimals > header->Decimals(fieldID)) header->Decimals(fieldID, nDecimals);
							}
						continue;
						}
					if (fieldSTR [i] == '-')
						{
						if (i > 0) header->Type (fieldID,DBVariableString);
						continue;
						}
					else header->Type (fieldID,DBVariableString);
					}
				if ((header->Type (fieldID) == DBVariableInt) && (fieldSTR [0] == '0')) header->Type (fieldID,DBVariableString);
				}
			}
		}
	for (fieldID = 0; fieldID < header->FieldNum(); fieldID++)
		{
		format [0] = '%';
		if (header->Length(fieldID) == 0) continue;
		switch (header->Type (fieldID))
			{
			default:
			case DBVariableString:
				header->Length (fieldID,0x01 << (int) (ceil (log2 ((float) header->Length (fieldID) + 1))));
				sprintf (format + 1,"%ds",header->Length(fieldID) + 1);
				fieldFLD = new DBObjTableField (header->Field (fieldID),header->Type (fieldID),format,header->Length (fieldID));
				break;
			case DBVariableInt:
				sprintf (format + 1,"%dd",header->Length(fieldID));
				fieldFLD = new DBObjTableField (header->Field (fieldID),header->Type (fieldID),format,sizeof (DBInt));
				break;
			case DBVariableFloat:
				sprintf (format + 1,"%d.%df",header->Length(fieldID),header->Decimals(fieldID));
				fieldFLD = new DBObjTableField (header->Field (fieldID),header->Type (fieldID),format,sizeof (DBFloat));
				break;
			}
		table->AddField (fieldFLD);
		}
	sprintf (format + 1,"0%dd",(int) ceil (log10 (recordNum) + 1));
	for (recordID = 0;recordID < recordNum; recordID++)
		{
		sprintf (recordName,format,recordID + 1);
		if ((recordPTR = table->Add(recordName)) == (DBObjRecord *) NULL)
			{
			CMmsgPrint (CMmsgAppError, "New Record Addition Error in: %s %d",__FILE__,__LINE__);
			ret = DBFault;
			goto Stop;
			}
		for (fieldID = 0; fieldID < header->FieldNum(); fieldID++)
			{
			if ((fieldFLD = table->Field(header->Field (fieldID))) == (DBObjTableField *) NULL) continue;
			if (records[recordID]->Field(fieldID) == (char *) NULL) continue;
			switch (header->Type(fieldID))
				{
				default:
				case DBVariableString:
					fieldFLD->String(recordPTR,records[recordID]->Field(fieldID));
					break;
				case DBVariableInt:
					if (sscanf (records[recordID]->Field(fieldID),"%d", &intVal)   != 1) intVal   = fieldFLD->IntNoData ();
					fieldFLD->Int(recordPTR,intVal);
					break;
				case DBVariableFloat:
					if (sscanf (records[recordID]->Field(fieldID),"%lf",  &floatVal) != 1) floatVal = fieldFLD->FloatNoData ();
					fieldFLD->Float(recordPTR,floatVal);
					break;					
				}
			}
		}
Stop:
	for (recordID = 0;recordID < recordNum; recordID++) delete records [recordID];
	if (records != (_DBImportASCIIRecord **) NULL) free (records);
	delete header;
	return (ret);
	}

DBInt DBExportASCIITable (DBObjTable *table, const char *fileName)

	{
	DBInt ret;
	FILE *fp;
	if ((fp = fopen (fileName,"w")) == (FILE *) NULL)
		{ CMmsgPrint (CMmsgSysError, "File Opening Error in: %s %d",__FILE__,__LINE__); return (DBFault); }
	ret = DBExportASCIITable (table,fp);
	fclose (fp);
	return (ret);
	}

DBInt DBExportASCIITable (DBObjTable *table, FILE *fp)

	{
	DBObjectLIST<DBObjTableField> *fields = table->Fields (); 
	DBObjRecord *record;
	DBObjTableField *field;
	
	fprintf (fp,"\"ID\"%c\"Name\"",DBASCIISeparator);
	for (field = fields->First ();field != (DBObjTableField *) NULL;field = fields->Next ())
		if ((strcmp (field->Name (),DBrNSymbol) == 0) || DBTableFieldIsVisible (field))
			fprintf (fp,"%c\"%s\"",DBASCIISeparator,field->Name ());
	fprintf (fp,"\n");
	for (record = table->First ();record != (DBObjRecord *) NULL;record = table->Next ())
		{
		if ((record->Flags () & DBObjectFlagIdle) == DBObjectFlagIdle) continue;
		fprintf (fp,"%d%c\"%s\"",record->RowID () + 1,DBASCIISeparator,record->Name ());
		for (field = fields->First ();field != (DBObjTableField *) NULL;field = fields->Next ())
			if ((strcmp (field->Name (),DBrNSymbol) == 0) || DBTableFieldIsVisible (field))
				switch	(field->Type ())
					{
					default:
					case DBTableFieldString:
						fprintf (fp,"%c\"%s\"",DBASCIISeparator,field->String (record));
						break;
					case DBTableFieldInt:
						if (field->Int (record) == field->IntNoData ())
								fprintf (fp,"%c",DBASCIISeparator);
						else	fprintf (fp,"%c%d",DBASCIISeparator,field->Int (record));
						break;
					case DBTableFieldFloat:
						if (CMmathEqualValues (field->Float (record),field->FloatNoData ()))
								fprintf (fp,"%c",DBASCIISeparator);
						else	fprintf (fp,"%c%f",DBASCIISeparator,field->Float (record));
						break;
					case DBTableFieldDate:
						fprintf (fp,"%c\"%s\"",DBASCIISeparator,field->String(record));
						break;
					}
		fprintf (fp,"\n");
		}
	return (DBSuccess);
	}