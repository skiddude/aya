<script>

    import { onMount } from "svelte"
    let { transport } = $props();

    let files = []

    onMount(() => {
        let parameters = new URLSearchParams(window.location.search)
        let entries = Array.from(parameters.entries())

        if (entries.length % 2 !== 0) {
            return
        }

        for (let i = 0; i < entries.length; i += 2) {
            let filepath = entries[i][1]
            let filename = entries[i + 1][1]

            files.push({ path: filepath, name: filename })
        }
    })
</script>

<div class="p-5">
    <div class="mb-3 flex items-center">
        <img src="./img/aya-studio.png" width="75" />
        <div class="ms-3 flex flex-col">
            <span class="text-2xl font-extrabold">Aya Studio</span>
            <code>v1.0.0</code>
        </div>
    </div>

    <span class="mb-2 text-lg font-bold">Recent Files:</span>
    <ul>
        {#each files as file}
            <li><a href={`#${file.path}`}>{file.name}</a></li>
        {/each}
    </ul>
</div>
