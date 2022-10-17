
void delete_clipboard_data ();

// Start the clipboard chars input, call it when the Paste key is pressed.
int new_clipboard_data ();
// Check the clipboard input data. Run it in the main loop.
int check_clipboard_data ();
// Put a text block to the clipboard. Use this to dump the text screen contents when the Copy key is pressed.
int put_text_to_clipboard (char *data);
// Put a graphics image to the clipboard. Use this when the Copy key is pressed.
int put_image_to_clipboard (uint32_t *data, int width, int height, int pitch);

