#pragma once

#include "ImageWriteTask.h"

class FKImageWriteTask : public IImageWriteTaskBase
{
public:

    float ServerWorldTime;
    FString ImgId;
    FString ClientID;
    bool bSaveToFile;
    bool bSendKafka;

    class UTackSnapshotSubsystem* SnapshotSubsystem;
    /** The filename to write to */
    FString Filename;

    /** The desired image format to write out */
    EImageFormat Format;

    /** A compression quality setting specific to the desired image format */
    int32 CompressionQuality;

    /** True if this task is allowed to overwrite an existing file, false otherwise. */
    bool bOverwriteFile;

    /** A function to invoke on the game thread when the task has completed */
    TFunction<void(bool)> OnCompleted;

    /** The actual write operation. */
    TUniquePtr<FImagePixelData> PixelData;

    /** Array of preprocessors to apply serially to the pixel data when this task is executed. */
    TArray<FPixelPreProcessor> PixelPreProcessors;

    FKImageWriteTask()
        : Format(EImageFormat::BMP)
        , CompressionQuality((int32)EImageCompressionQuality::Default)
        , bOverwriteFile(true)
    {
    }

    //not sure what having the api thing at the front does
    virtual bool RunTask() override final;
    virtual void OnAbandoned() override final;

private:

    /**
     * Run the task, attempting to write out the raw data using the currently specified parameters
     *
     * @return true on success, false on any failure
     */
    bool WriteToDisk();

    /**
     * Ensures that the desired output filename is writable, deleting an existing file if bOverwriteFile is true
     *
     * @return True if the file is writable and the task can proceed, false otherwise
     */
    bool EnsureWritableFile();

    /**
     * Initialize the specified image wrapper with our raw data, ready for writing
     *
     * @param InWrapper      The wrapper to initialize with our data
     * @param WrapperFormat  The desired image format to write out
     * @return true on success, false on any failure
     */
    bool InitializeWrapper(IImageWrapper* InWrapper, EImageFormat WrapperFormat);

    /**
     * Special case implementation for writing bitmap data due to deficiencies in the IImageWriter API (it can't set raw pixel data without trying to compress it, which asserts)
     *
     * @return true a bitmap was written out at the specified filename, false otherwise
     */
    bool WriteBitmap();


    /**
     * Run over all the processors for the pixel data
     */
    void PreProcess();
};