<script>
    import { ChatStyle, getChatStyleName } from "@/Enums/ChatStyle"
    import { GearType, getGearTypeName, getGearTypeIcon } from "@/Enums/GearType"

    import Button from "@/Controls/Button.svelte"
    import TextInput from "@/Controls/TextInput.svelte"
    import Card from "@/Components/Card.svelte"

    import Select from "@/Controls/Select.svelte"
    import InputLabel from "@/Controls/InputLabel.svelte"
    import Checkbox from "@/Controls/Checkbox.svelte"
    import TextArea from "@/Controls/TextArea.svelte"

    let { transport } = $props();

    let showServerSettings = $state(false)
    let isTransitioning = $state(false)
    let portValue = $state("")
    let fileName = $state(null)
    let serverPassword = $state(null)
    let serverName = $state("")
    let serverHost = $state("")
    let serverDescription = $state("")
    let chatStyle = $state(ChatStyle.ClassicAndBubble)
    let progress = $state(false)
    let error = $state(false)
    let broadcast = $state(false)
    let masterServerUrl = $state("")
    let masterServerKey = $state("")

    let isPublicDomain = $state(false)

    let maxPlayers = $state(16)

    let enabledGearTypes = $state(Object.values(GearType).map(() => false))
    
    let gearAttributes = $derived(enabledGearTypes.reduce((acc, isEnabled, index) => {
        return isEnabled ? acc | (1 << index) : acc
    }, 0))

    function handleGearChange(index, isChecked) {
        enabledGearTypes[index] = isChecked
    }

    function toggleSettings() {
        isTransitioning = true
        setTimeout(() => {
            showServerSettings = !showServerSettings
            isTransitioning = false
        }, 100)
    }

    function handleFileSelect(event) {
        const file = event.target.files[0]
        if (file) {
            fileName = file.name
            console.log("Selected file:", file.name)
        }
    }

    function startServer() {
        progress = true
        setTimeout(() => (progress = false), 5000)
        error = true
    }
</script>

{#if showServerSettings}
    <div class="lex h-full w-full flex-col transition-opacity duration-300 ease-in-out" class:opacity-0={isTransitioning} class:opacity-100={!isTransitioning}>
        <div class="flex flex-shrink-0 items-center p-1">
            <button onclick={toggleSettings} class="aya-anim-pop flex h-10 items-center justify-center rounded px-3 font-medium text-neutral-500 transition duration-100 hover:bg-black/10 hover:text-neutral-600 dark:text-neutral-300 dark:hover:bg-white/10">
                <i class="fa fa-solid fa-fw fa-arrow-left me-2"></i>
                <span>Back</span>
            </button>
        </div>

        <div class="flex-1 px-3 pb-24 pt-1">
            <Card class="flex flex-col">
                <div>
                    <InputLabel for="max_players" value="Maximum amount of players" />
                    <TextInput class="mt-2" id="max_players" type="number" bind:value={maxPlayers} required />
                </div>

                <div class="mt-3">
                    <InputLabel for="server_password" value="Server password (optional)" />
                    <TextInput class="mt-2" id="server_password" type="password" placeholder="Leave empty for no password" bind:value={serverPassword} />
                </div>

                <div class="mt-3">
                    <InputLabel for="chat_style" value="Chat style" />
                    <Select bind:value={chatStyle}>
                        {#each Object.values(ChatStyle) as style}
                            <option value={style} selected={chatStyle === style || null}>{getChatStyleName(style)}</option>
                        {/each}
                    </Select>
                </div>

                <div class="mt-3">
                    <Checkbox name="is_public_domain" label="Allow connected players to download your place" bind:checked={isPublicDomain} />
                </div>

                <div class="mt-1">
                    <Checkbox name="broadcast" label="Broadcast server details to a masterserver" bind:checked={broadcast} />
                </div>

                {#if broadcast}
                    <div class="mt-2 select-none text-sm font-medium italic text-red-500 dark:text-red-400">Details about your server (IP address, port, connected players, etc.) will be publicly visible on the masterserver and to anyone who connects to it. Please make sure you trust the masterserver you are connecting to.</div>

                    <div class="mt-3">
                        <InputLabel for="master_server_url" value="Masterserver base URL" />
                        <TextInput class="mt-2" id="master_server_url" type="text" placeholder="e.g. masterserver.example.com" bind:value={masterServerUrl} required />
                    </div>

                    <div class="mt-3">
                        <InputLabel for="master_server_key" value="Masterserver authentication key (optional)" />
                        <TextInput class="mt-2" id="master_server_key" type="text" bind:value={masterServerKey} />
                    </div>

                    <div class="mt-3">
                        <InputLabel for="server_name" value="Server Name" />
                        <TextInput class="mt-2" id="server_name" placeholder="Max 50 characters" type="text" bind:value={serverName} required />
                    </div>

                    <div class="mt-3">
                        <InputLabel for="server_host" value="Server Host" />
                        <TextInput class="mt-2" id="server_host" placeholder="Typically your username" type="text" bind:value={serverHost} required />
                    </div>

                    <div class="mt-3">
                        <InputLabel for="server_description" value="Server Description" />
                        <TextArea class="mt-2" id="server_description" placeholder="Max 1500 characters" bind:value={serverDescription} required />
                    </div>
                {/if}

                <div class="mt-3">
                    <InputLabel for="name" value="Allowed gears" />
                    <div class="mt-1 grid grid-cols-2 gap-1 md:grid-cols-3">
                        {#each Object.entries(GearType) as [typeName, typeValue], index}
                            <Checkbox name={getGearTypeName(typeValue).toString().toLowerCase().replace(" ", "_")} bind:checked={enabledGearTypes[index]} onchange={() => handleGearChange(index, enabledGearTypes[index])}>
                                {getGearTypeName(typeValue)}
                                <i class="fa-regular fa-fw fa-{getGearTypeIcon(typeValue)} ms-1"></i>
                            </Checkbox>
                        {/each}
                    </div>
                </div>
            </Card>
        </div>
    </div>
{:else}
    <!-- Main Page -->
    <div class="flex h-full w-full flex-col items-center justify-center transition-opacity duration-300 ease-in-out" class:opacity-0={isTransitioning} class:opacity-100={!isTransitioning}>
        <img src="./img/aya-server{progress ? '-progress' : error ? '-error' : ''}.png" width="150" alt="Aya Server" class="pointer-events-none select-none" />

        <span class="my-5 select-none text-4xl font-extrabold text-stone-900 dark:text-stone-50">Aya Server</span>

        <div class="flex w-full flex-col items-center">
            <div class="flex w-full max-w-64 flex-col">
                <label for="place-file" class="aya-anim-pop flex cursor-pointer items-center rounded rounded-b-none border border-neutral-500/20 bg-neutral-500/5 px-3 py-1 text-lg text-neutral-500/85 transition duration-100 hover:bg-neutral-500/10">
                    <div class="select-none">
                        <i class="fa-light fa-folder-open fa-sm fa-fw me-1"></i>
                        {fileName ?? "Select place file …"}
                    </div>
                </label>

                <input id="place-file" type="file" accept=".rbxl,.ayal,.rbxl.gz,.ayal.gz" class="hidden" onchange={handleFileSelect} />

                <TextInput bind:value={portValue} min="0" max="65535" type="number" placeholder="Port (e.g. 53640)" class="!h-auto !rounded-t-none !border-t-0 py-1 text-lg" />
            </div>

            <button class="aya-anim-pop mt-2 flex h-auto items-center justify-center rounded px-2 py-1 text-center text-neutral-500 transition duration-100 hover:bg-black/10 hover:text-neutral-600 dark:text-neutral-300 dark:hover:bg-white/10" onclick={toggleSettings}>
                <i class="fa-regular fa-square-sliders me-0.5"></i>
                Server Settings
            </button>
        </div>

        <Button onclick={startServer} disabled={progress} icon="fa-play" class="mt-4 !w-auto !min-w-max !max-w-full !gap-1 !rounded-md !border !border-green-600 !bg-green-500 !shadow-lg dark:!border-green-700 dark:!bg-green-600 {progress ? 'pointer-events-none cursor-default opacity-50' : ''}" text="Start" />
    </div>
{/if}
