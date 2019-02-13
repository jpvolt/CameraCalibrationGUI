//https://stackoverflow.com/questions/22702630/converting-cvmat-to-sdl-texture
SDL_Surface* convertCV_MatToSDL_Surface(const cv::Mat &matImg)
{
    IplImage opencvimg2 = (IplImage)matImg;
    IplImage* opencvimg = &opencvimg2;

     //Convert to SDL_Surface
    SDL_Surface* frameSurface = SDL_CreateRGBSurfaceFrom(
                         (void*)opencvimg->imageData,
                         opencvimg->width, opencvimg->height,
                         opencvimg->depth*opencvimg->nChannels,
                         opencvimg->widthStep,
                         0xff0000, 0x00ff00, 0x0000ff, 0);

    return frameSurface;
}