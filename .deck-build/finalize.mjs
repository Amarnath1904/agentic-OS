import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {Presentation, PresentationFile} from '@oai/artifact-tool';

const root=path.resolve('..');
const out=path.join(root,'output');
const build=path.join(root,'.deck-build');
const skill='C:/Users/pande/.codex/plugins/cache/openai-primary-runtime/presentations/26.1011.11113/skills/presentations';
const {finalizePresentation}=await import(pathToFileURL(path.join(skill,'container_tools/artifact_tool_utils.mjs')).href);
let result=await finalizePresentation({workspaceDir:root,candidatePath:path.join(build,'candidate.pptx'),finalPath:path.join(out,'Agentic_OS_MVP_Presentation.pptx'),pythonExecutable:'C:/Users/pande/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',integrityValidatorPath:path.join(skill,'container_tools/inspect_presentation_package_integrity.py'),layoutValidatorPath:path.join(skill,'container_tools/inspect_presentation_layout_geometry.py'),layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-bullet-geometry','--validate-heading-fit'],explicitTotalSlideCount:39,fontPolicy:{basis:'design',families:['Arial','Courier New']},verifyArtifactToolImport:true,receiptPath:path.join(build,'validation-final.json')});
console.log(JSON.stringify(result));
