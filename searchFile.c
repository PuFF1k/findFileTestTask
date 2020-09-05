#include"searchFile.h"
#include<stdio.h>
#include<string.h>

#include<pthread.h>
#include<stdlib.h>
#include "dirent.h"

struct dataForThread
{
    unsigned int numberOfDirs;
    wchar_t* searchedFile;
    wchar_t** dirs;
};

void addSpaceForFolder(struct dataForThread* dft)
{
    if (dft->numberOfDirs > 1) 
    {
        wchar_t** newDirsp = malloc(dft->numberOfDirs * sizeof(wchar_t*));
        memcpy(newDirsp, dft->dirs, sizeof(dft->dirs) * (dft->numberOfDirs - 1));
        free(dft->dirs);
        dft->dirs = newDirsp;
    }
    else if (dft->numberOfDirs == 1) 
    {
        dft->dirs = malloc(dft->numberOfDirs * sizeof(wchar_t*));
    }
}

pthread_t tid[8];
struct dataForThread dft[8];
unsigned char fileFound = 0;
wchar_t searchedFilePath[PATH_MAX];
pthread_mutex_t lock;

void recursiveSearch(const wchar_t* dirPath, unsigned int indent, wchar_t* searchedFile)
{
    WDIR* pdr;
    struct wdirent* pde;

    pthread_mutex_lock(&lock);
    if (fileFound == 1) 
    {
        pthread_mutex_unlock(&lock);
        return;
    }
    pthread_mutex_unlock(&lock);

    if (!(pdr = wopendir(dirPath)))
    {
        //printf("Could not open directory: %ls\n", name);
        //too many files that are DT_DIR and cannot be opened so dont print nothing
        return;
    }
        

    while ((pde = wreaddir(pdr)) != NULL)
    {
        switch (pde->d_type) {
        case DT_REG:
        {
            if (wcscmp(pde->d_name, searchedFile) == 0)
            {
                pthread_mutex_lock(&lock);
                fileFound = 1;
                swprintf(searchedFilePath, sizeof(searchedFilePath), L"%ls", pdr->patt);
                swprintf(searchedFilePath + wcslen(pdr->patt) - 1, sizeof(searchedFilePath) - wcslen(pdr->patt) - 1,L"%ls", pde->d_name);
                pthread_mutex_unlock(&lock);
                goto Cleanup;
            }
            break;
        }
        case DT_DIR:
        {
            wchar_t path[PATH_MAX];
            if (wcscmp(pde->d_name, L".") == 0 || wcscmp(pde->d_name, L"..") == 0)
            {
                continue;
            }
            swprintf(path, sizeof(path), L"%ls/%ls", dirPath, pde->d_name);
            recursiveSearch(path, indent + 2, searchedFile);
        }
        }
    }

    Cleanup:
    wclosedir(pdr);
}


void* threadedSearch(void* arg) 
{
    struct dataForThread* myData = (struct dataForThread*)arg;

    for (unsigned int i = 0; i < myData->numberOfDirs; i++) 
    {
        recursiveSearch(myData->dirs[i], 0, myData->searchedFile);
        free(myData->dirs[i]);
    }
    free(myData->dirs);
}



wchar_t* searchFileMain(wchar_t* fileToFind)
{
    struct wdirent* pde;
    unsigned int dCounter = 0;
    unsigned char tCounter = 0;
#ifdef __linux__ 
    WDIR* pdr = wopendir(L"/");
#elif _WIN32
    WDIR* pdr = wopendir(L"c:/");
#else

#endif
    

    if (pdr == NULL)
    {
        printf("Could not open root(for windows \"c:/\") directory\n");
        return 0;
    }
    
    while ((pde = wreaddir(pdr)) != NULL)
    {
        switch (pde->d_type) {
        case DT_REG:
        {
            if (wcscmp(pde->d_name, fileToFind) == 0) 
            {
                pthread_mutex_lock(&lock);
                fileFound = 1;
                swprintf(searchedFilePath, sizeof(searchedFilePath), L"%ls", pdr->patt);
                swprintf(searchedFilePath + wcslen(pdr->patt) - 1, sizeof(searchedFilePath) - wcslen(pdr->patt) - 1, L"%ls", pde->d_name);
                pthread_mutex_unlock(&lock);
                goto Cleanup;
            }
            break;
        }

        case DT_DIR:
        {
            if (dCounter == 0)
            {
                dft[tCounter].numberOfDirs = 0;
                dft[tCounter].searchedFile = malloc((wcslen(fileToFind) + 1) * sizeof(wchar_t));
                wcscpy(dft[tCounter].searchedFile, fileToFind);
            }

            dft[tCounter].numberOfDirs++;
            addSpaceForFolder(&dft[tCounter]);
            dft[tCounter].dirs[dft[tCounter].numberOfDirs - 1] = malloc(PATH_MAX * sizeof(wchar_t));
            wcscpy(dft[tCounter].dirs[dft[tCounter].numberOfDirs - 1], pdr->patt);
            wcscpy(dft[tCounter].dirs[dft[tCounter].numberOfDirs - 1] + wcslen(pdr->patt) - 1, pde->d_name);

            tCounter++;
            break;
        }
        }
        
        if (tCounter > 7)
        {
            tCounter = 0;
            dCounter++;
        }
    }

    for (char i = 0; i < 8; i++) 
    {
        pthread_create(&(tid[i]), NULL, &threadedSearch, &dft[i]);
    }

    for (char i = 0; i < 8; i++)
    {
        pthread_join(tid[i], NULL);
    }

    Cleanup:
    wclosedir(pdr);
    return searchedFilePath;
}