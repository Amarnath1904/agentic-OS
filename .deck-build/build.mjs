import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {Presentation, PresentationFile} from '@oai/artifact-tool';

const root=path.resolve('..');
const out=path.join(root,'output');
const build=path.join(root,'.deck-build');
const skill='C:/Users/pande/.codex/plugins/cache/openai-primary-runtime/presentations/26.1011.11113/skills/presentations';
const C={bg:'#180B2B',panel:'#2A1547',purple:'#7D43D5',light:'#BE98F6',white:'#FFFFFF',muted:'#D5C9E6',ink:'#251139',pale:'#F2EBFC'};
const p=Presentation.create({slideSize:{width:1280,height:720}});
let s, dark=true, count=0;
function text(t,x,y,w,h,size=26,color,bold=false,font='Arial',align='left'){
 const o=s.shapes.add({geometry:'textbox',name:t.slice(0,44),position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});
 o.text=t;o.text.style={typeface:font,fontSize:size,color:color||(dark?C.white:C.ink),bold,alignment:align,verticalAlignment:'middle',insets:0,wrap:'word'};return o;
}
function rect(x,y,w,h,fill=C.panel,line=C.purple){return s.shapes.add({geometry:'rect',position:{left:x,top:y,width:w,height:h},fill,line:{fill:line,width:1.4}})}
function node(t,x,y,w=220,h=80){let o=rect(x,y,w,h,dark?C.panel:C.pale);text(t,x+15,y+9,w-30,h-18,24,undefined,true,'Arial','center');return o}
function line(x1,y1,x2,y2,arrow=false,dashed=false){return s.shapes.add({geometry:'connector',kind:'straight',position:{left:Math.min(x1,x2),top:Math.min(y1,y2),width:Math.abs(x2-x1),height:Math.abs(y2-y1),horizontalFlip:x2<x1,verticalFlip:y2<y1},line:{fill:dark?C.light:C.purple,width:2,style:dashed?'dashed':'solid'},tail:arrow?{type:'triangle'}:{type:'none'}})}
function slide(title,section='FOUNDATIONS',notes='',light=false){count++;dark=!light;s=p.slides.add();s.background.fill=dark?C.bg:C.white;text(section,64,34,1050,24,16,dark?C.light:C.purple,true);text(title,64,92,1152,100,44,undefined,true);text('AGENTIC OS  /  MVP TECHNICAL WALKTHROUGH',64,673,1050,20,13,dark?C.muted:C.purple);text(String(count).padStart(2,'0'),1158,669,58,26,17,dark?C.light:C.purple,false,'Arial','right');s.speakerNotes.text=notes;return s}
function note(t){text(t,64,593,1152,53,23,dark?C.muted:C.purple)}
function flow(labels,caption,y=290){const w=(1152-(labels.length-1)*38)/labels.length;labels.forEach((v,i)=>{node(v,64+i*(w+38),y,w,112);if(i<labels.length-1)line(64+i*(w+38)+w,y+56,64+(i+1)*(w+38)-7,y+56,true)});if(caption)note(caption)}
function rows(items,x=64,y=226,w=1152,gap=112){items.forEach((a,i)=>{text(a[0],x,y+i*gap,Math.min(320,w*.3),45,28,dark?C.light:C.purple,true);text(a[1],x+Math.min(350,w*.33),y+i*gap,w-Math.min(350,w*.33),75,27)})}
function code(t,x=64,y=231,w=1152,h=300,size=26){rect(x,y,w,h,'#10071D','#5D377E');text(t,x+26,y+18,w-52,h-36,size,C.white,false,'Courier New')}
function seq(actors,messages,caption){let xs=actors.map((_,i)=>140+i*1000/(actors.length-1));actors.forEach((a,i)=>{node(a,xs[i]-105,213,210,62);line(xs[i],284,xs[i],575,false,true)});let step=288,dy=Math.min(54,270/messages.length);messages.forEach(([a,b,t],i)=>{let y=step+i*dy+30;line(xs[a],y,xs[b],y,true);let left=Math.min(xs[a],xs[b]);text(t,left+13,y-29,Math.abs(xs[b]-xs[a])-26,27,19,dark?C.white:C.ink)});if(caption)note(caption)}
const src=(files,extra='')=>'Repository sources: '+files+'. '+extra;
const design='Completed-state design assumption requested for this presentation. Gemini gateway, agent registry, tool policy and guest application transport are not implemented in the inspected repository. The diagram describes their coherent integration, not a tested runtime. ';

slide('Agentic OS','MVP  /  TECHNICAL WALKTHROUGH','A technical walkthrough of the supplied x86 OS MVP and the user-requested completed-state agent architecture. No runtime success or performance measurements are claimed.');
text('A small operating system\nwith an agent control layer',64,216,730,150,49,undefined,true);
node('Intent',860,218,285,80);node('Main OS agent',860,351,285,80);node('File operations',860,484,285,80);line(1002,298,1002,343,true);line(1002,431,1002,476,true);text('Kernel architecture, Gemini networking,\nfile CRUD and a practical run guide',64,432,700,90,28,C.muted);

slide('Walkthrough map','OVERVIEW','The presentation separates kernel mechanics, networking and the completed-state agent design, then provides repository-grounded build and demo commands.',true);
rows([['01  Foundation','Boot, memory, input and the shell'],['02  Connectivity','NE2000 networking and the Gemini gateway'],['03  Agents','Simple agents and controlled file operations'],['04  Run & demonstrate','Build, configure, launch and verify']],64,208,1152,94);

slide('Kernel and agent layers','OVERVIEW',src('kernel/kernel.c; kernel/shell/shell.c; kernel/net/tcp_probe.c',design));
text('OS foundation',64,229,500,50,32,C.light,true);text('x86 kernel, RAMFS, shell,\nNE2000 and TCP probe',64,302,510,122,35);
text('Agent control layer',680,229,535,50,32,C.light,true);text('Gemini gateway, agent creation\nand a main OS file agent',680,302,535,122,35);
note('Gemini interprets requests. The main OS agent executes scoped file operations.');

slide('A request becomes an OS operation','OVERVIEW',design+'Example: the user requests creation of a short note. The model selects a declared tool. A deterministic local executor validates the path and changes the filesystem.');
flow(['User request','Agent decision','Validated tool','RAM filesystem'],'Example: “Create /notes/demo.txt with a short project summary.”');

slide('System architecture','FOUNDATIONS',src('kernel/kernel.c; kernel/fs/ramfs/ramfs.c; kernel/net/ne2k.c',design));
node('Shell and user request',64,230,340,90);node('Main OS agent',470,230,340,90);node('Host Gemini gateway',876,230,340,90);line(404,275,462,275,true);line(810,275,868,275,true);
node('RAMFS tool executor',470,390,340,85);line(640,320,640,382,true);node('C kernel + x86 assembly',64,528,1152,60);line(640,475,640,520,true);text('Guest',64,362,260,40,27,C.light,true);text('Host owns HTTPS\nand model credentials',892,363,318,94,25,C.muted);

slide('Repository map','FOUNDATIONS',src('build.sh; linker.ld; kernel/*; host_tools/tcp_ping/*'),true);
rows([['Boot and build','boot.asm, kernel/entry.asm, build.sh, linker.ld'],['Kernel services','memory/, interrupts/, drivers/, scheduler/, syscalls/'],['User-facing features','auth/, io/, shell/, fs/ramfs/'],['Networking','kernel/net/ and host_tools/tcp_ping/']],64,209,1152,94);

slide('Build pipeline','FOUNDATIONS',src('build.sh','The build assembles ELF32 stubs, compiles freestanding C, links the kernel at 0x10000 and converts ELF to a flat binary. It computes the number of sectors for the bootloader and pads the image.'));
flow(['NASM +\nC sources','ELF32\nkernel.elf','Raw bytes\nkernel.bin','Disk image\nboot.img'],'The BIOS loader needs the flat kernel bytes, not an ELF header.');

slide('Boot sequence','FOUNDATIONS',src('boot.asm; kernel/entry.asm; kernel/kernel.c','The loader uses BIOS INT 13h extensions, enables A20, loads the GDT, sets CR0.PE and jumps to 0x10000. Entry code initializes the stack and clears BSS.'));
seq(['BIOS','Boot sector','Kernel entry','C kernel'],[[0,1,'Load boot sector at 0x7C00'],[1,2,'Load kernel, enter 32-bit mode'],[2,3,'Clear BSS, then call kmain()']],'Execution moves from 16-bit BIOS code to the 32-bit C kernel.');

slide('Kernel initialization order','FOUNDATIONS',src('kernel/kernel.c','kmain initializes hardware, memory, interrupts and scheduler before showing the banner. login_run gates access. net_init follows login, then RAMFS initialization and the infinite shell loop.'));
flow(['Hardware\nVGA + keys','Memory\nheap + pages','Interrupts\nGDT + IDT','Scheduler\nstate setup']);
text('Then: login, network initialization, RAMFS initialization and shell',90,472,1100,80,30,C.light,true);

slide('Memory layout','FOUNDATIONS',src('linker.ld; kernel/entry.asm; kernel/memory/memory.c; kernel/memory/paging.c','The first 8 MiB are identity-mapped through two page tables of 1024 4 KiB pages. The heap spans 4–8 MiB. kfree checks and clears allocations but the allocator is a bump allocator with no general reuse.'),true);
const mem=[['0x00010000','Kernel code, data and BSS'],['0x000B8000','VGA text memory'],['0x00400000','Heap starts at 4 MiB'],['0x00800000','Heap ends at 8 MiB']];
mem.forEach((a,i)=>{node(a[0],64,216+i*86,280,65);text(a[1],386,216+i*86,795,65,29)});note('4 KiB pages. A 16 KiB kernel stack. Files allocate their contents from the heap.');

slide('Keyboard input and screen output','FOUNDATIONS',src('kernel/drivers/keyboard.c; kernel/drivers/vga.c; kernel/io/line_input.c; kernel/interrupts/pic.c','PIC remaps IRQ vectors and unmasks keyboard IRQ1. Network receive uses polling. VGA output writes text to the text buffer. The line-input helper supplies editable lines and controls echo.'));
flow(['PS/2\nkeyboard','IRQ1\nhandler','Line input\nbuffer','Shell\ndispatch'],'VGA text output displays the prompt, command results and errors.');

slide('Shell command surface','FILESYSTEM',src('kernel/shell/shell.c','Input capacity is 256 bytes and command capacity is 32 bytes, both including terminators. The current directory begins at /. Arguments are parsed by whitespace. This shell does not implement pipes or a full quoting language.'),true);
rows([['Navigate','help    pwd    cd    ls'],['Create and read','mkdir    touch    cat'],['Update and delete','write    rm'],['Network probe','tcpping <ipv4> [port]']],64,210,1152,94);

slide('RAM filesystem model','FILESYSTEM',src('kernel/fs/ramfs/ramfs.h; kernel/fs/ramfs/ramfs.c; kernel/fs/ramfs/path.h','The filesystem holds up to 96 entries. Each stores a used flag, node type, absolute path, data pointer and size. The path buffer is 128 bytes; validation can reject paths before full capacity. Root is implicit and lookup scans entries. Reboot discards the filesystem.'));
node('/',82,266,150,75);node('/notes',345,266,245,75);node('demo.txt',728,228,360,75);node('ideas.txt',728,370,360,75);line(232,304,337,304,true);line(590,304,718,267,true);line(590,304,718,407,true);
text('96 entry slots',72,473,390,70,40,C.light,true);text('RAM-backed data\nCleared on restart',680,479,510,90,30);note('Each file stores a normalized path, byte length and pointer to its contents.');

slide('File CRUD maps directly to RAMFS','FILESYSTEM',src('kernel/shell/shell.c; kernel/fs/ramfs/ramfs.c','Create rejects an existing path. Write creates or replaces a file and ensures parent directories. Reads use offset/capacity. Unlink refuses root and nonempty directories. These are kernel functions, not completed POSIX file descriptors.'));
rows([['CREATE','touch  /  ramfs_create_empty_file()'],['READ','cat  /  ramfs_read_file()'],['UPDATE','write  /  ramfs_write_file()'],['DELETE','rm  /  ramfs_unlink()']],64,209,1152,94);

slide('File write sequence','FILESYSTEM',src('kernel/shell/shell.c; kernel/fs/ramfs/ramfs.c','The shell splits path from remaining text and resolves an absolute path. RAMFS creates parents when needed, allocates new data before replacing existing data, and returns a status. The heap is finite.'));
seq(['User','Shell','RAMFS','Heap'],[[0,1,'write /notes/demo.txt hello'],[1,2,'Resolve path and call write'],[2,3,'Allocate and copy new bytes'],[3,2,'Return buffer'],[2,1,'Replace entry data, return status']],'The shell handles text. RAMFS owns the file state.');

slide('Runtime boundaries','FOUNDATIONS',src('kernel/scheduler/scheduler.c; kernel/syscalls/syscall.c; kernel/kernel.c','Scheduler code rotates bookkeeping state but does not implement a register/stack context switch. The shell does not return to kernel_loop. SYS_READ and SYS_EXIT contain unfinished behavior. This is not evidence of a working isolated multi-process runtime.'),true);
rows([['Execution','The interactive shell runs inside the kernel'],['Scheduling','Task structures exist; agent jobs are logical workflows'],['Storage','Files live in RAM and disappear after restart']]);
note('The MVP keeps the execution model small and explicit.');

slide('Guest networking stack','NETWORKING',src('kernel/net/ne2k.c; kernel/net/tcp_probe.c; kernel/net/net_config.h','NE2000 is configured at I/O 0x300 and IRQ11, while packet reception is polled. The probe builds Ethernet, ARP, IPv4 and TCP packets. It does not provide an HTTP or TLS client.'));
flow(['NE2000\nI/O driver','Ethernet\nframes','ARP + IPv4\naddressing','TCP SYN\nprobe'],'tcpping checks a listening TCP port with a SYN-ACK response.');

slide('Network configuration','NETWORKING',src('kernel/net/net_config.h; bochsrc.bxrc','These addresses are compiled defaults, not a guarantee that the selected Bochs network backend provides that subnet. Match them to the chosen network. Changing net_config.h requires a rebuild.'),true);
rows([['Device','NE2000 at 0x300, IRQ 11'],['Guest address','10.0.2.15 / 255.255.255.0'],['Gateway','10.0.2.2'],['MAC address','B0:C4:20:00:00:01']],64,209,1152,94);

slide('TCP probe sequence','NETWORKING',src('kernel/net/tcp_probe.c','The next hop is the destination on the local subnet or the configured gateway. The probe broadcasts ARP, sends SYN, checks reply addresses, ports, flags and acknowledgement, then sends RST+ACK to end the probe. It does not transfer an application payload.'));
seq(['Shell','Guest stack','Next hop','TCP service'],[[0,1,'tcpping <ipv4> <port>'],[1,2,'ARP request / reply resolves MAC'],[1,3,'TCP SYN'],[3,1,'TCP SYN-ACK'],[1,3,'TCP RST-ACK'],[1,0,'ok (SYN-ACK)']],'A successful probe establishes reachability, not Gemini API access.');

slide('Host-side connectivity tools','NETWORKING',src('host_tools/tcp_ping/__main__.py; host_tools/tcp_ping/http_server.py; host_tools/tcp_ping/tcp_probe.py','The Python utility uses the host operating system TCP connect API. It is separate from the guest SYN probe. It serves /health, /status and GET/POST /ping on 127.0.0.1:8765 by default. No outbound probe occurs until requested.'));
node('CLI or HTTP client',64,281,300,90);node('Python TCP probe',493,281,300,90);node('Target IPv4:port',922,281,294,90);line(364,326,485,326,true);line(793,326,914,326,true);text('/health     /status     /ping',64,456,1152,60,34,C.light,true);note('Host diagnostics provide a separate way to inspect connectivity.');

slide('Gemini networking area','AGENT ARCHITECTURE',design+'This design uses a host gateway to perform DNS, HTTPS, authentication and model request handling. A guest application transport carries bounded messages. A SYN probe alone cannot carry these messages.');
node('Agentic OS guest',64,290,290,110);node('Host gateway',495,290,290,110);node('Gemini API',926,290,290,110);line(354,345,487,345,true);line(785,345,918,345,true);text('Application\nmessages',360,245,130,65,19,C.muted);text('HTTPS',805,280,106,37,22,C.light,true);note('The guest owns file operations. The host gateway owns cloud communication.');

slide('Gateway responsibilities','AGENT ARCHITECTURE',design+'Sources: https://ai.google.dev/gemini-api/docs/api-key and https://ai.google.dev/gemini-api/docs/function-calling. API credentials stay in the host environment. Gemini returns function names and arguments; application code executes those functions. Exact timeout, retry and request limits are deployment settings.',true);
rows([['Credentials','GEMINI_API_KEY stays on the host'],['Model requests','Prompt, allowed tools and bounded context'],['Transport','HTTPS, timeout handling and response parsing'],['Execution boundary','Validated tool requests return to the OS']],64,209,1152,94);

slide('Gemini tool-call sequence','AGENT ARCHITECTURE',design+'Source: https://ai.google.dev/gemini-api/docs/function-calling. The application supplies tool definitions, receives a requested call, runs its own implementation, and submits the result for the model response. Local policy remains authoritative.');
seq(['OS agent','Gateway','Gemini','File tools'],[[0,1,'Request + allowed tool schema'],[1,2,'Send model request'],[2,1,'Function name + arguments'],[1,3,'Validate and execute through OS'],[3,1,'Structured tool result'],[1,2,'Submit tool result'],[2,1,'Final response'],[1,0,'Return final result']],'The model proposes a tool call. The runtime decides whether to execute it.');

slide('A simple agent is a configuration','AGENT ARCHITECTURE',design+'An agent configuration names a purpose, tools, scope and execution budget. It does not create a separate kernel process. The sample is a design-level configuration, not an existing file format or command.');
code('name: notes-helper\npurpose: maintain short project notes\ntools: file.read, file.update, file.list\nroot: /notes\nmax_steps: 4',64,221,714,326,25);text('One purpose\nA small tool set\nAn explicit file scope',847,259,365,215,30,C.light,true);note('The main OS agent registers and runs each configuration.');

slide('Agent creation sequence','AGENT ARCHITECTURE',design+'Creation validates configuration and stores it in a registry. The main agent returns an identifier. The registry can be RAM-backed in this MVP, so it should not imply persistent agents across reboot.');
seq(['User','Main OS agent','Policy','Registry'],[[0,1,'Create a notes helper'],[1,2,'Check purpose, tools and file scope'],[2,1,'Accept constrained configuration'],[1,3,'Store agent definition'],[3,1,'Return agent ID'],[1,0,'Agent ready to receive a task']],'Agent creation defines a reusable workflow with limited permissions.');

slide('Main OS agent','AGENT ARCHITECTURE',design+'The main agent resolves user intent, selects a helper, applies operation policy and delegates validated tool execution. Results include actual tool status rather than only generated prose. It remains a logical control layer above the current kernel filesystem.');
node('User task',64,240,270,85);node('Main OS agent',476,240,335,85);node('Gemini gateway',930,240,285,85);line(334,282,468,282,true);line(811,282,922,282,true);
node('Agent registry',64,464,270,85);node('Policy + executor',476,464,335,85);node('RAMFS',930,464,285,85);line(642,325,642,456,true);line(334,505,468,505,true);line(811,505,922,505,true);

slide('File tool contract','AGENT ARCHITECTURE',design+'The names are presentation-level interfaces, not existing kernel exports. Create maps to ramfs_create_empty_file; read maps to ramfs_read_file; update maps to ramfs_write_file; delete maps to ramfs_unlink. Tool policy must check existence before update to make update-only semantics distinct from RAMFS upsert.',true);
rows([['file.create','path, content  /  create a new file'],['file.read','path, offset, limit  /  return bytes'],['file.update','path, content  /  replace an existing file'],['file.delete','path  /  remove the named file']],64,209,1152,94);

slide('Agent file-operation sequence','AGENT ARCHITECTURE',design+'Example flow for a request to create a note. The tool adapter enforces create-only behavior and maps errors. Read-back verifies the bytes before the agent reports success. This is a designed sequence rather than a recorded execution.');
seq(['User','OS agent','Tool policy','RAMFS'],[[0,1,'Create /notes/demo.txt'],[1,2,'file.create(path, content)'],[2,3,'Validate scope, create and write'],[3,2,'Status + bytes written'],[2,3,'Read back saved content'],[3,2,'Return saved bytes'],[2,1,'Verified tool result'],[1,0,'File created']],'A successful model response alone does not prove a filesystem change.');

slide('Operation checks','AGENT ARCHITECTURE',design+'Scope checking belongs in the agent adapter. Existing RAMFS normalizes paths, rejects dot segments, enforces types and refuses root deletion or nonempty-directory deletion. Agent allowlists and user confirmation are design assumptions.');
flow(['Tool\nallowlist','Normalized\npath + scope','Type + size\nvalidation','Execute +\nread back'],'Delete requests identify the exact path and pass the configured confirmation rule.');

slide('Failure and recovery behavior','AGENT ARCHITECTURE',design+'Timeouts return an explicit network error. Invalid tool output is rejected before mutation. Read-only model requests may retry within a budget. Mutating operations need request IDs and reconciliation to avoid duplicate effects. These policies describe the requested integrated design.',true);
rows([['Initial model timeout','Report the failure before any file operation'],['Invalid tool call','Reject unknown tools or malformed arguments'],['Lost write response','Inspect file state before repeating the operation'],['Missing file','Return a filesystem error with the affected path']],64,207,1152,96);

slide('Run environment','BUILD & RUN',src('build.sh; bochsrc.bxrc; host_tools/tcp_ping/__main__.py','Use a Bash environment with NASM and an i686-elf GCC/binutils cross toolchain. WSL or a Linux development environment is suitable if those tools are installed. The supplied emulator configuration uses Windows Bochs 3.0 paths. No clean kernel build or guest runtime was verified during deck creation.'));
rows([['Build machine','Bash, NASM and i686-elf GCC/binutils'],['Emulator','Bochs with BIOS, VGA BIOS and NE2000 support'],['Host utilities','Python 3 for host_tools.tcp_ping']]);note('Run build commands from the repository root.');

slide('Source and configuration preflight','BUILD & RUN',src('boot.asm; bochsrc.bxrc','The inspected boot.asm contains the malformed instruction "mo   v ds, ax". Change it to "mov ds, ax" before rebuilding. The checked-in bochsrc points to a different project directory. These are required setup corrections; the presentation does not modify repository source.'),true);
rows([['Boot assembly','Correct “mo   v ds, ax” to “mov ds, ax”'],['Disk image','Point ata0-master to this project’s boot.img'],['BIOS files','Match romimage and vgaromimage to your installation'],['Network backend','Match vnet/tap settings to the guest subnet']],64,207,1152,96);

slide('Build the boot image','BUILD & RUN',src('build.sh','Commands are intended for Bash with the cross toolchain installed. The script overwrites generated boot/kernel build artifacts. It links kernel.elf, uses objcopy for kernel.bin, assembles boot.bin with the calculated sector count, concatenates and pads boot.img.'));
code('nasm -v\ni686-elf-gcc --version\ni686-elf-objcopy --version\n\nbash build.sh',64,223,720,320,27);text('Output files',846,233,340,48,30,C.light,true);text('kernel.elf\nkernel.bin\nboot.bin\nboot.img',846,305,355,203,29);note('Resolve the preflight issues before running the build.');

slide('Launch Bochs','BUILD & RUN',src('bochsrc.bxrc; kernel/auth/login.c; kernel/shell/shell.c','PowerShell launch uses the installed Bochs executable. Keep the local config disk path and BIOS paths correct. Use the development credentials defined in kernel/auth/login.c; passwords are deliberately not copied into this shareable presentation.'));
code('& "C:\\Program Files\\Bochs-3.0\\bochs.exe" `\n  -q -f .\\bochsrc.bxrc',64,227,1152,130,26);
flow(['BIOS boot','Kernel banner','Login gate','agentic:/$'],'Use the development login configured in kernel/auth/login.c.',420);

slide('A short filesystem demo','BUILD & RUN',src('kernel/shell/shell.c; kernel/fs/ramfs/ramfs.c','These commands exercise the existing shell in order. write uses the remainder of the input line as content and replaces file content. No shell quotes are required. The final directory should be empty. Reboot resets RAMFS.'));
code('mkdir /notes\ntouch /notes/demo.txt\nwrite /notes/demo.txt Hello Agentic OS\ncat /notes/demo.txt\nwrite /notes/demo.txt Updated note\ncat /notes/demo.txt\nrm /notes/demo.txt\nls /notes',64,217,816,373,25);text('Create\nRead\nUpdate\nDelete',944,249,240,275,34,C.light,true);note('Expected reads: “Hello Agentic OS”, then “Updated note”.');

slide('Run the host network service','BUILD & RUN',src('host_tools/tcp_ping/__main__.py; host_tools/tcp_ping/http_server.py','Run the server in one host terminal and the health request in another. The default listener is localhost only. A guest cannot reach the host service via the guest loopback address. Do not assume the host HTTP service is a Gemini bridge.'));
code('python -m host_tools.tcp_ping serve\n\n# In another PowerShell terminal\nInvoke-RestMethod http://127.0.0.1:8765/health',64,227,1152,225,26);text('Health response',64,501,400,45,30,C.light,true);text('{ "status": "ok" }',476,501,714,45,30);note('For interactive probes: python -m host_tools.tcp_ping shell');

slide('Connectivity and agent demo','BUILD & RUN',src('kernel/shell/shell.c; kernel/net/net_config.h',design+'First select a known listening endpoint reachable from the guest subnet. The agent prompts are a completed-state demonstration script, not supported shell commands in this checkout. Host gateway launch commands cannot be given until its actual entry point exists.'));
text('Guest connectivity',64,218,600,45,30,C.light,true);code('tcpping <reachable-ipv4> <port>',64,280,1152,80,26);
text('Agent demonstration prompts',64,397,900,45,30,C.light,true);text('“Create a notes helper limited to /notes.”\n“Create, read, update and delete /notes/demo.txt.”',64,467,1152,95,29);note('Gateway configuration: host API key, selected model, guest endpoint and allowed tools.');

slide('Troubleshooting map','BUILD & RUN',src('boot.asm; build.sh; bochsrc.bxrc; kernel/net/net_config.h; kernel/net/tcp_probe.c; kernel/fs/ramfs/ramfs.c','These are source-grounded diagnostic leads, not a claim that the demo passed. A SYN-ACK says the port is reachable. It does not test TLS, Gemini authentication or tool execution.'),true);
rows([['Assembler error','Check the boot.asm instruction and NASM installation'],['Boot loop or blank screen','Check image path, raw kernel format and loaded sectors'],['tcpping has no reply','Check NIC settings, subnet, next hop and listening port'],['Files vanish on reboot','RAMFS is volatile by design']],64,207,1152,96);

slide('End-to-end walkthrough','CLOSING',src('kernel/kernel.c; kernel/shell/shell.c; kernel/fs/ramfs/ramfs.c',design+'The final demonstration should show boot, a direct shell CRUD cycle, networking reachability, an agent definition and a verified tool-driven file action. Avoid claiming production security, persistence, full TCP/TLS support or measured reliability from this MVP. Sources for provider behavior: https://ai.google.dev/gemini-api/docs/function-calling and https://ai.google.dev/gemini-api/docs/api-key.'));
flow(['Boot the OS','Reach the\nnetwork','Create an\nagent','Verify file\nchanges']);text('A visible chain from user intent\nto a concrete filesystem result',64,474,1145,103,42,C.light,true);

count++;dark=true;s=p.slides.add();s.background.fill=C.bg;
text('AGENTIC OS',64,48,1152,34,20,C.light,true);
text('Thank you',64,244,1152,130,84,C.white,true,'Arial','center');
text('Questions & discussion',64,397,1152,60,30,C.muted,false,'Arial','center');
text('MVP TECHNICAL WALKTHROUGH',64,673,1050,20,13,C.muted);
text('40',1158,669,58,26,17,C.light,false,'Arial','right');
s.speakerNotes.text='Thank the audience and invite questions about the architecture, agent workflows and running the MVP.';

await fs.mkdir(path.join(build,'renders'),{recursive:true});
await fs.mkdir(out,{recursive:true});
await (await PresentationFile.exportPptx(p)).save(path.join(build,'candidate.pptx'));
await fs.writeFile(path.join(build,'deck.json'),JSON.stringify(p.toProto()));
for(let i=0;i<p.slides.items.length;i++){
 if (i !== 39) continue;
 const png=await p.export({slide:p.slides.items[i],format:'png',scale:1});
 await fs.writeFile(path.join(build,'renders',`slide-${String(i+1).padStart(2,'0')}.png`),new Uint8Array(await png.arrayBuffer()));
 console.log('Rendered',i+1);
}
const {finalizePresentation}=await import(pathToFileURL(path.join(skill,'container_tools/artifact_tool_utils.mjs')).href);
let result=await finalizePresentation({workspaceDir:root,candidatePath:path.join(build,'candidate.pptx'),finalPath:path.join(out,'Agentic_OS_MVP_Final.pptx'),pythonExecutable:'C:/Users/pande/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',integrityValidatorPath:path.join(skill,'container_tools/inspect_presentation_package_integrity.py'),layoutValidatorPath:path.join(skill,'container_tools/inspect_presentation_layout_geometry.py'),layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-bullet-geometry','--validate-heading-fit'],explicitTotalSlideCount:40,fontPolicy:{basis:'design',families:['Arial','Courier New']},verifyArtifactToolImport:true,receiptPath:path.join(build,'validation-thank-you.json')});
console.log(JSON.stringify(result));





