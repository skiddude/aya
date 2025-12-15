<script>
    import { preventDefault } from 'svelte/legacy';

    import { onMount } from "svelte";

    import Button from "@/Controls/Button.svelte"
    import InputLabel from "@/Controls/InputLabel.svelte"
    import TextInput from "@/Controls/TextInput.svelte"
    import InputError from "@/Controls/InputError.svelte"
    import Card from "@/Components/Card.svelte"
    
    let { transport } = $props();

    let masterServerURL = $state("")
    let masterServerKey = $state("")
    let serverHostPassword = $state("")
    let robloSecurityCookie = $state("")
    let error = $state("")

    onMount(() => {
        getSettings();
    });

    async function getSettings() {
        masterServerURL = await transport.getMasterServerURL() ?? ""
        robloSecurityCookie = await transport.getRobloSecurityCookie() ?? ""
    }

    async function submit() {
        let a = '';
        (Object.getOwnPropertyNames(transport)).forEach(element => {
            a += (element) + "\n"
        });

        try {
            transport.setMasterServerURL(masterServerURL)
            transport.setRobloSecurityCookie(robloSecurityCookie)
        } catch (err) {
            error = err + "\n" + a
        }
    }
</script>

<div class="grid h-full grid-rows-[auto,1fr] gap-3">
    <div class="col-span-1 grid grid-cols-3 gap-3">
        This page is temporary.
        <Card title="General Settings" class="col-span-1" icon="fa-gear">
            <form onsubmit={preventDefault(submit)}>
                <div>
                    <InputLabel for="masterserverurl" value="Master Server URL" />
                    <TextInput placeholder="http://masterserver.com/" class="mt-1" id="masterserverurl" type="text" bind:value={masterServerURL} />
                </div>

                <div>
                    <InputLabel for="masterserverkey" value="Master Server Key" />
                    <TextInput placeholder="http://masterserver.com/" class="mt-1" id="masterserverkey" type="password" bind:value={masterServerKey} />
                </div>

                <div>
                    <InputLabel for="hostserverpassword" value="Host Server Password" />
                    <TextInput placeholder="" class="mt-1" id="hostserverpassword" type="password" bind:value={serverHostPassword} />
                </div>

                <div>
                    <InputLabel for="securitycookie" value="ROBLOSECURITY Cookie" />
                    <TextInput placeholder=".ROBLOSECURITY=_|WARNING:-DO-NOT-SHARE-THIS.--Sharing-this-will-allow-someone-to-log-in-as-you-and-to-steal-your-ROBUX-and-items.|" class="mt-1" id="securitycookie" type="password" bind:value={robloSecurityCookie} />
                </div>

                {#if error}
                    <InputError message={error} />
                {/if}

                <div class="mt-2 flex w-full justify-end">
                    <Button class="aya-btn-sm" type="submit" text="Update Settings" icon="fa-gear" title="Update Settings" />
                </div>
            </form>
        </Card>
    </div>
</div>