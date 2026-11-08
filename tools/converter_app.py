"""Simple end-user SD-card converter; entry point for the standalone Windows app."""
from pathlib import Path
import queue,threading,json,sys
import tkinter as tk
from tkinter import ttk,filedialog,messagebox
from convert_labels import convert

def convert_card(card,progress=None):
    card=Path(card).resolve()
    if not card.is_dir():raise ValueError('Select an accessible SD-card folder.')
    if not any((card/name).is_dir() for name in ('Games','ROMS','GBA','GB','GBC','Labels')):
        raise ValueError('Select the SD-card root containing Games, GBA/GB/GBC, or Labels.')
    return convert(card,card,card,allow_card_output=True,progress=progress)

def run_gui():
    win=tk.Tk();win.title('slot. advance label converter');win.resizable(False,False)
    win.configure(bg='#eeeae0');style=ttk.Style(win)
    if 'vista' in style.theme_names():style.theme_use('vista')
    body=ttk.Frame(win,padding=24);body.grid()
    ttk.Label(body,text='slot. advance',font=('Segoe UI',20,'bold')).grid(row=0,column=0,columnspan=2,sticky='w')
    ttk.Label(body,text='Labels and cartridge colours, ready for your GBA.',font=('Segoe UI',10)).grid(row=1,column=0,columnspan=2,sticky='w',pady=(3,18))
    ttk.Label(body,text='1. Put your Slot / Cart Studio Labels folder on the SD card.\n2. Select the card below, then convert.',justify='left').grid(row=2,column=0,columnspan=2,sticky='w',pady=(0,14))
    path=tk.StringVar();status=tk.StringVar(value='Choose an SD card to begin.');jobs=queue.Queue();busy=False
    entry=ttk.Entry(body,textvariable=path,width=53,state='readonly');entry.grid(row=3,column=0,sticky='ew',padx=(0,8))
    def choose():
        selected=filedialog.askdirectory(parent=win,title='Select your SD-card root')
        if selected:path.set(selected);status.set('Ready to convert. Games, saves and source labels stay in place.')
    picker=ttk.Button(body,text='Select SD card...',command=choose);picker.grid(row=3,column=1)
    bar=ttk.Progressbar(body,mode='determinate',length=490);bar.grid(row=4,column=0,columnspan=2,sticky='ew',pady=(20,8))
    ttk.Label(body,textvariable=status,wraplength=490,justify='left').grid(row=5,column=0,columnspan=2,sticky='w')
    ttk.Label(body,text='Realistic and simple cartridges are written to SYSTEM/SLOT on the selected card.\nNo online account or Python installation is needed.',foreground='#64635f',wraplength=490).grid(row=7,column=0,columnspan=2,sticky='w',pady=(18,0))
    def worker(selected):
        try:
            result=convert_card(selected,lambda done,total,name:jobs.put(('progress',done,total,name)))
            jobs.put(('done',result))
        except Exception as error:jobs.put(('error',str(error)))
    def run():
        nonlocal busy
        if not path.get():choose()
        if not path.get():return
        busy=True;button.config(state='disabled');picker.config(state='disabled')
        status.set('Reading labels, shell colours and ROM headers...');bar.config(mode='indeterminate');bar.start(16)
        threading.Thread(target=worker,args=(path.get(),),daemon=True).start()
    button=ttk.Button(body,text='Convert labels and shells',command=run);button.grid(row=6,column=0,columnspan=2,sticky='ew',pady=(18,0),ipady=5)
    def poll():
        nonlocal busy
        try:
            while True:
                event=jobs.get_nowait()
                if event[0]=='progress':
                    _,done,total,name=event;bar.stop();bar.config(mode='determinate',maximum=total,value=done);status.set(f'{done} / {total}   {name}')
                else:
                    bar.stop();busy=False;button.config(state='normal');picker.config(state='normal')
                    if event[0]=='done':
                        result=event[1];number=len(result['items']);bar.config(mode='determinate',maximum=max(1,number),value=number)
                        status.set(f'Done. {number} cartridges converted. Safely eject your SD card.')
                        if result['warnings']:messagebox.showwarning('Conversion finished with notes','\n'.join(result['warnings']),parent=win)
                    else:status.set('Conversion stopped. Completed artwork is safe to keep.');messagebox.showerror('Could not finish conversion',event[1],parent=win)
        except queue.Empty:pass
        win.after(100,poll)
    def about():
        messagebox.showinfo('About slot. advance','slot. advance label converter v1.0\n\nDS Style by FrankieT19. slot. by Brandon Kowalski. Cartridge artwork by Markus.\n\nGPL-3.0-or-later. Corresponding source, asset notices and licences are included in the project download.',parent=win)
    ttk.Button(body,text='About',command=about).grid(row=8,column=1,sticky='e',pady=(12,0))
    def close():
        if busy:
            messagebox.showinfo('Conversion running','Please let the conversion finish before closing or removing the SD card.',parent=win);return
        win.destroy()
    win.protocol('WM_DELETE_WINDOW',close);win.after(100,poll);win.mainloop()

def main():
    # A non-UI smoke test for the bundled executable, used on a disposable card.
    if '--self-test-card' in sys.argv:
        at=sys.argv.index('--self-test-card');card=Path(sys.argv[at+1]);result=convert_card(card)
        (card/'converter-app-test.json').write_text(json.dumps({'converted':len(result['items']),'frozen':bool(getattr(sys,'frozen',False))}),encoding='utf-8');return
    run_gui()
if __name__=='__main__':main()
