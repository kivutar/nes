int cpustep(void);
u8int	memread(u16int);
void	memwrite(u16int, u8int);
u8int	ppuread(u16int);
void	ppuwrite(u16int, u8int);
void	ppustep(void);
void	loadstate(char *);
void	savestate(char *);
void	message(char *, ...);
void	put8(u8int);
int	get8(void);
void	apustep(void);
void	initaudio(void);
void	audiosample(void);
int	audioout(void);
void	dmcstep(void);
