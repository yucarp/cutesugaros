struct CharDevice *CreateCharDevice(struct Directory *root, struct CharDeviceFunctions cd){
    struct CharDevice *char_device = malloc(sizeof(struct CharDevice));
    char_device->functions.write_char_function = cd.write_char_function;
    DirectoryAddChild(root, (void *) char_device);
    return char_device;
}

char WriteChar(struct CharDevice *cd, long offset){
    return cd->functions.write_char_function(cd, offset);
}
