# armory-hardened
Armory DIY Hardware Wallet - User Guide (usage; installation to follow)

## Warning
* Keep in mind this thing is totally beta.
* The device is only tested under Windows (8.1) and with Armory 0.92.3
* The device is not yet access-restrictable. Keep itas physically secure as a paper wallet.
* Always keep a backup of the root keys as before. The device is meant to make your keys secure against the internet, but not yet safe against physical theft/damage/weird beta behavior.

But: The device can do nothing wrong with your money or transactions. The random number k needed for signing is generated securely and deterministically according to RFC6979. The worst thing to happen is the signing to fail due to a whatever bug, but then the transaction is just invalid and will not be accepted by Armory. It will not reveal your keys.

##Interface
The device features 4 buttons:
* 2 mechanical buttons labeled SW0 and SW1
* 2 capacitive “Q-Touch” buttons labeled QTB0 and QTB1

The QTB buttons are used for browsing menus and info (up/down). The display shows “QTB” in the center bottom if available, with “<” and/or “>” to indicate whether you can scroll up with QTB0 and down with QTB1.

The SW buttons are used for confirmation/canceling actions or to go back and forth in menu structures. The current function will be shown in the right and/or left bottom as “SW0:” and “SW1:” with a character following, indicating the assigned action:
* S: Select
* B: Back
* Y: Yes
* N: No
* R: Refresh

##Start up
Be sure to insert an SD card. Connect to the computer and make sure it gets acknowledged as a removable drive. Make sure the card is formatted with FAT (FAT16).
 
The device starts up with no wallet set up, thus telling you “Wallet: None”.
With SW1 you go to the wallet setup menu to proceed with setting up the first wallet.

If you have at least one wallet set up, the home screen will show you the currently active wallet and the option to open the latest unsigned tx file (SW0) as well as to go to the main menu (SW1).

##Wallet setup
In the main menu, go to “Set up wallet” by scrolling down with QTB1 and select it with SW1:S. If there is no wallet set up yet, just press SW1 on the home screen. 
There are four options yet:

###1. Create on-chip
The device will create a new wallet with root key data gathered from two floating ADC pins. For 256 samples the LSB and L+1SB of every sample are XOR’ed and put together as a key. True randomness is not proven in any way!

If you use this feature, it is advised to write down the root key (see “Wallet export”) as a backup and test its integrity via “Shuffled root key file”.
 
###2. Plain wallet file
Using this function, the device will search for the file ending with “.decrypt.wallet” on the SD card with the most recent modification date.

The file has to be a standard decrypted Armory wallet file, from which the root key will be extracted.

###3. Plain root key file
The device will search for the file containing the string “rootkey” anywhere in its name (e.g. just rootkey.txt) on the SD card with the most recent modification date.

The file has to be an ASCII text file and must contain the 2 times 9 words from a normal Armory paper backup. The device will ignore all spaces and line breaks (\r and \n).

###4. Shuffled root key file
Works the same way as with a plain root key file, but with obfuscated characters. Use “Show shuffle code” in the main menu (or on the home screen if there is no wallet yet).

The available Armory conform charset (asdfghjkwertuion) will be randomly replaced with the hex charset (0123456789ABCDEF, uppercase!) in ASCII.

Example: If your first backup word is “krfh” and the device displays k=6, r=8, f=7, h=C among the 16 characters, type in “687C” as the first word in your text file. The device will swap that back internally according to the current shuffle map, but no malware on your computer will get to know your plain key.

After every successful import, the code will be automatically re-shuffled. You can always trigger that yourself with pressing SW1:R in the “Shuffle code” display.

###Import confirmation
After finding a valid file, the device will compute and show the corresponding Armory wallet ID and ask you whether you want to import or not. Confirm with SW1:Y or abort with SW0:N.

A freshly set up wallet will automatically be set as the active one. Look at “Select Wallet” how to switch between multiple wallets.

###Note for imports via files
After a successful import, the file on the card will be completely overwritten with “-” (ASCII 0x2D) and then dropped from FAT. You will get an error message on the screen if secure erasing is unsuccessful. If the import fails itself (e.g. invalid file content), the file will persist and needs to be deleted manually.

###Finishing the setup
If the imported wallet is not already existing in Armory on the computer, you can use for example the watching-only data file export function (see “Wallet Export”).

After the wallet was set up in Armory itself, you have to permanently provide a watching-only wallet file on the SD card for the device to look up public keys. Do this by opening the wallet in Armory and click “Export Watching-Only Copy” -> “Export Watching-Only Wallet File” and save the file to the device’s SD card. It has to contain the wallet ID as well as the ending “.watchonly.wallet” in the filename.
Now you are set to use this wallet with Armory and Armory Hardened!

##Wallet Erasing
You have to select the wallet via “Select Wallet” first before you can erase it.

Go to “Erase wallet” in the main menu and select it with SW1:S. The device will show you the Armory wallet ID of the wallet going to be erased and ask for your confirmation. Confirm with SW1:Y or abort with SW0:N.

After erasing, the remaining wallet with the lowest internal slot number will be set as active. If there is no other wallet left, you will end with the “Wallet: None” home screen.

##Wallet Export
You have to select the wallet via “Select Wallet” first before you can export its data.

To make backups and to conveniently import watch-only wallets into Armory, the device offers few export modes:

###Watchonly data file 
This will write an Armory-compatible watching-only wallet file named “armory_{walletid}.hardened.rootpubkey” to the SD card. This file does not contain any security-sensitive data. 

You can import it to Armory via “Import or Restore Wallet” -> “Import watching-only wallet data” -> “Continue” -> “Load From Text File”.

###Rootkey file
This will write an ASCII text file named “armory_.decryt.rootkey.txt” to the SD card. It contains the full, plain wallet seed as if you made a single-sheet paper backup in Armory. 

You can import it to Armory via “Import or Restore Wallet” -> “Single-Sheet Backup (printed)” -> “Continue” and copy the file content into the shown input fields.

Be careful using that function on an unsecure computer!

###Show rootkey
Shows the plain wallet seed on the screen instead of writing it to a file. Use QTB0/1 to scroll through the 6 pages needed to display it.

##Transaction signing
Before you can sign a transaction, you have to set the wallet this tx is coming from as active on the device. 

If it is not already, do so by going to the main menu and select “Select wallet”. This will show you all set-up wallet IDs ordered by the internal storage order, scrollable with QTB0/1. Select the desired wallet with SW1:S and wait for the wallet parameters to be computed from the root key on-the-fly.

You will then be brought back to the home screen, displaying the newly selected wallet. This selection is stored permanently, so unplugging the device will not reset it.

Pressing SW0 on the home screen will search the SD card for the file with the most recent modification date containing “.unsigned.tx” in the filename.

You will see the Armory transaction ID, which you should compare to the one shown by Armory to be sure the file was not modified in between.

Use QTB0 to scroll down and see the number of inputs and total input value, the fee, and all outputs with amount and recipient address (well, the first 18 characters due to space restrictions on the screen).

All amounts get rounded to the next micro-BTC, milli-BTC or BTC depending on the order of magnitude the amount is in.

You then can abort with SW0:N or confirm with SW1:Y. After confirmation, the screen will show you the progress as the number of inputs already signed.

The signed transaction is written to the SD card as a new file named “armory_{txID}.hardened.SIGNED.tx”. Load it with Armory and broadcast.

The old, unsigned file gets deleted but not erased.
