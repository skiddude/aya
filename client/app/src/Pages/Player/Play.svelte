<script>
    import { preventDefault } from 'svelte/legacy';

    import ServerBrowser from "@/Components/ServerBrowser.svelte"
    import Button from "@/Controls/Button.svelte"
    import InputLabel from "@/Controls/InputLabel.svelte"
    import TextInput from "@/Controls/TextInput.svelte"
    import InputError from "@/Controls/InputError.svelte"
    import Card from "@/Components/Card.svelte"

    let { transport, onNavigate } = $props();

    let ipAddress = $state("")
    let port = $state("")
    let error = $state("")

    async function submit() {
        try {
            const baseUrl = transport.getMasterServerURL()
            const userId = Math.floor(Math.random() * 65535)

            let bodyColors = []
            let charApp = []

            let charAppValue = transport.characterAppearanceJSON()

            console.error("initial: " + charAppValue);

            let selectedAssets = JSON.parse(charAppValue);

            if (selectedAssets.hat) {
                selectedAssets.hat.forEach(function (assetId) {
                    charApp.push(assetId);
                });
            }

            if (selectedAssets.normal) {
                for (const subCategory in selectedAssets.normal) {
                    if (selectedAssets.normal.hasOwnProperty(subCategory)) {
                        const assetIds = selectedAssets.normal[subCategory];
                        assetIds.forEach(function (assetId) {
                            charApp.push(assetId);
                        });
                    }
                }
            }

            console.error("charapp: " + JSON.stringify(charApp))

            let bodyColorsValue = transport.getBodyColorJson()
            bodyColors = JSON.parse(bodyColorsValue);
            
            const finalCharApp = baseUrl + '/avatar-fetch/?userid=' + userId + '&json=' + encodeURIComponent(JSON.stringify(charApp)) + '&body=' + encodeURIComponent(JSON.stringify(bodyColors));

            const connectionObj = {
                ClientPort: 0,
                MachineAddress: ipAddress,
                ServerPort: Number(port),
                UserName: "Player",
                DisplayName: "Player",
                CharacterAppearance: finalCharApp,
                GameId: 1818,
                PlaceId: 1818,
                PingInterval: 20,
                UserId: userId,
                CreatorId: 1,
                MembershipType: "None",
                SuperSafeChat: false,
                IsUnknownOrUnder13: false,
                CreatorTypeEnum: "User",
                ChatStyle: "ClassicAndBubble",
                VirtualVersion: 0,
                IsRobloxPlace: true
            }

            if (connectionObj.VirtualVersion === 4) {
                transport.enableChatBarWidget()
            }

            await transport.launchGame(JSON.stringify(connectionObj), connectionObj.VirtualVersion)
        } catch (err) {
            error = "Failed to connect to server: " + err.message
        }
    }
</script>

<div class="grid h-full grid-rows-[auto,1fr] gap-3">
    <div class="col-span-1 grid grid-cols-2 gap-3">
        <Card title="Direct Connect" class="col-span-1" icon="fa-plug">
            <form onsubmit={preventDefault(submit)}>
                <div>
                    <InputLabel for="ip" value="IP Address" />
                    <TextInput placeholder="127.0.0.1" class="mt-1" id="ip" type="text" bind:value={ipAddress} required />
                </div>

                <div class="mt-2">
                    <InputLabel for="port" value="Port" />
                    <TextInput placeholder="53640" class="mt-1" id="port" type="number" bind:value={port} required />
                </div>

                {#if error}
                    <InputError message={error} />
                {/if}

                <span class="mt-2 block text-sm italic text-neutral-500 dark:text-neutral-300">
                    You will be joining as <b>Player</b>.
                    <button type="button" onclick={() => onNavigate('avatar')} class="underline">Customize Character</button>
                </span>

                <div class="mt-2 flex w-full justify-end">
                    <Button class="aya-btn-sm !bg-transparent !text-pink-400 !duration-0 hover:!bg-transparent hover:!text-pink-500" title="Add to Favorites" icon="fa-lg fa-heart" />
                    <Button class="aya-btn-sm" type="submit" text="Join" icon="fa-right-to-bracket" title="Connect to Server" />
                </div>
            </form>
        </Card>
        <Card title="Play Solo" class="col-span-1" icon="fa-play">placeholder</Card>
    </div>

    <ServerBrowser {transport} />
</div>
