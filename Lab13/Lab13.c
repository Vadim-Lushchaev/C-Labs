#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct id3_record ID3v2Tag;
typedef struct ID3 ID3v2Frame;

struct ID3
{
    char id[5];
    unsigned long size;
    unsigned char flags[2];
    unsigned char encoding;
    unsigned char* value;
    ID3v2Frame* next;
};

struct id3_record
{
    unsigned char signature[3];
    unsigned char version[2];
    unsigned char flags;
    unsigned long size;
    ID3v2Frame* first;
};

ID3v2Tag* CreateTag()
{
    ID3v2Tag* tag = (ID3v2Tag*)malloc(sizeof(ID3v2Tag));
    memset(tag->signature, 0, 4);
    tag->size = 0;
    memset(tag->version, 0, 2);
    tag->flags = 0;
    tag->first = NULL;
    return tag;
}

unsigned long syncsafeBEtolong(unsigned long val)
{
    unsigned long tmp = (val & 0xFF000000) >> 24 |
        (val & 0x00FF0000) >> 8 |
        (val & 0x0000FF00) << 8 |
        (val & 0x000000FF) << 24;

    unsigned long res1 = (tmp & 0x0000007F) |
        ((tmp & 0x00007F00) >> 1) |
        ((tmp & 0x007F0000) >> 2) |
        ((tmp & 0x7F000000) >> 3);
    return res1;
}

int ReadTag(ID3v2Tag* dst, FILE* file)
{
    char tagName[5] = {0};
    int size;
    unsigned long bytesReaded = 0;

    if (!file) return -3;

    fseek(file, 0, SEEK_SET);
    int read_bytes = fread(dst->signature, 1, 3, file);

    if (strcmp("ID3", dst->signature) != 0)
    {
        return -1;
    }

    dst->version[0] = fgetc(file);
    dst->version[1] = fgetc(file);
    dst->flags = fgetc(file);

    fread(&size, 1, 4, file);
    dst->size = syncsafeBEtolong(size);

    ID3v2Frame *fr = NULL, *prev = NULL;
    while (bytesReaded < dst->size)
    {
        fr = (ID3v2Frame*)malloc(sizeof(ID3v2Frame));

        fread(tagName, 1, 4, file);

        if (tagName[0] == 0) { break; }

        fr->next = NULL;
        strcpy(fr->id, tagName);
        fread(&size, sizeof(char), 4, file);
        fr->size = syncsafeBEtolong(size);
        fr->flags[0] = fgetc(file);
        fr->flags[1] = fgetc(file);

        fr->value = (unsigned char*)malloc(sizeof(unsigned char) * fr->size);
        fread(fr->value, sizeof(unsigned char), fr->size, file);
        memcpy_s(fr->value, fr->size, fr->value + 1, fr->size - 1);
        fr->value[fr->size] = 0;
        bytesReaded += fr->size;
        if (prev == 0)
            dst->first = fr;
        else
            prev->next = fr;
        prev = fr;
        bytesReaded += 10;
    }
    return 0;
}

void ShowTag(ID3v2Tag* tag)
{
    if (!tag)
    {
        printf("<empty> \n");
        return;
    }
    ID3v2Frame* fr = tag->first;
    while (fr != NULL)
    {
        printf("%s: %s\n", fr->id, fr->value);
        fr = fr->next;
    }
}

char* GetFrame(ID3v2Tag* tag, char* name)
{
    ID3v2Frame* fr = tag->first;
    while (fr != NULL)
    {
        if (strcmp(fr->id, name) == 0)
        {
            return fr->value;
        }
        fr = fr->next;
    }
    return NULL;
}

void SetFrameValue(ID3v2Tag** tag, char* name, char* value)
{
    ID3v2Frame* fr = (*tag)->first, *pre = NULL;
    while (fr != NULL)
    {
        if (strcmp(fr->id, name) == 0)
        {
            break;
        }
        pre = fr;
        fr = fr->next;
    }
    // Не нашли подходящий для установки frame
    if (pre->next == NULL)
    {
        pre->next = (ID3v2Frame*)malloc(sizeof(ID3v2Frame));
        pre->next->next = NULL;
        strncpy(pre->next->id, name, 4);
        pre->next->id[4] = 0;
        pre->next->flags[1] = 1;
        
        pre->next->value = (unsigned char*)malloc(strlen(value) + 1);
        memcpy(pre->next->value, value, strlen(value));
        pre->next->size = strlen(value);
        pre->next->value[pre->next->size] = 0;
    }
    else
    {
        fr->value = NULL;
        free(fr->value);
        fr->value = (unsigned char*)malloc(strlen(value) + 1);
        memcpy(fr->value, value, strlen(value));
        fr->size = strlen(value);
        fr->value[fr->size] = 0;
    }
    
}

int main(int argc, char* argv[])
{
    FILE* in = fopen("CYGO_-_Panda_E.mp3", "rb");
    ID3v2Tag* header = CreateTag();
    ReadTag(header, in);
    SetFrameValue(&header, "TTTT", "1231233");
    ShowTag(header);
    return 0;
}
