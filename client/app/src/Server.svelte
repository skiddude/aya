<script>

    import PageButton from "@/Components/ServerPageButton.svelte"
    import HostPage from "@/Pages/Server/Host.svelte"
    import JobPage from "@/Pages/Server/Jobs.svelte"
    import REPLPage from "@/Pages/Server/REPL.svelte"
    let { transport } = $props();

    let selectedPage = $state("host")
    let isTransitioning = $state(false)

    const pages = [
        { icon: "fa-tower-broadcast", id: "host", title: "Host" },
        { icon: "fa-server", id: "jobs", title: "Jobs" },
        { icon: "fa-rectangle-terminal", id: "repl", title: "REPL" }
    ]

    function changePage(newPage) {
        if (newPage === selectedPage) return

        isTransitioning = true
        setTimeout(() => {
            selectedPage = newPage
            isTransitioning = false
        }, 100)
    }
</script>

<div class="flex h-screen w-screen flex-col items-center justify-center dark:bg-stone-950 dark:text-neutral-100">
    <div class=" contents-center absolute bottom-0 left-0 z-50 flex w-full items-center justify-center gap-2 border-t border-gray-300 bg-gray-100 py-1.5 dark:border-neutral-950 dark:bg-neutral-900">
        {#each pages as page}
            <PageButton {page} {selectedPage} onClick={() => changePage(page.id)} />
        {/each}
    </div>
    <main class="h-full w-full overflow-scroll">
        <div class="h-full w-full transition-opacity duration-300 {isTransitioning ? 'opacity-0' : 'opacity-100'}">
            {#if selectedPage === "host"}
                <HostPage {transport} />
            {:else if selectedPage === "jobs"}
                <JobPage {transport} />
            {:else if selectedPage === "repl"}
                <REPLPage {transport} />
            {/if}
        </div>
    </main>
</div>
