<script>
    import { spring } from "svelte/motion"

    import PageButton from "@/Components/PlayerPageButton.svelte"
    import PlayPage from "@/Pages/Player/Play.svelte"
    import AvatarPage from "@/Pages/Player/Avatar.svelte"
    import LevelsPage from "@/Pages/Player/Levels.svelte"
    import PackagesPage from "@/Pages/Player/Packages.svelte"
    import FavoritesPage from "@/Pages/Player/Favorites.svelte"
    import SettingsPage from "@/Pages/Player/Settings.svelte"
    import AboutPage from "@/Pages/Player/About.svelte"
    
    let { transport } = $props();

    let selectedPage = $state("play")

    const pages = [
        { id: "play", title: "Play" },
        { id: "avatar", title: "Character" },
        { id: "levels", title: "Levels" },
        { id: "packages", title: "Packages" },
        { id: "favorites", title: "Favorites" },
        { id: "server", title: "Open Aya Server" },
        { id: "studio", title: "Open Aya Studio" },
        { id: "settings", title: "Settings", icon: "fa-cog" },
        { id: "about", title: "About", icon: "fa-circle-info" }
    ]

    let pageIndicator = spring(9.5, {
        stiffness: 0.15,
        damping: 0.7
    })

    $effect(() => {
        if (selectedPage) {
            const activeButton = document.querySelector(`[data-page="${selectedPage}"]`)
            if (activeButton) {
                pageIndicator.set(activeButton.offsetTop + 2)
            }
        }

    });
</script>

<div class="flex h-screen">
    <div class="flex w-[3.5rem] flex-col items-center border-r border-gray-300 bg-gray-100 pb-0.5 pt-2 text-white dark:border-neutral-950 dark:bg-neutral-900">
        <div class="absolute left-0 h-6 w-[0.175rem] rounded-r bg-aya-500" style="transform: translateY({$pageIndicator}px); transition: height 0.2s ease" role="presentation"></div>
        {#each pages as page}
            {#if page.id == "server"}
                <div class="flex-grow"></div>
                <PageButton {page} {selectedPage} onClick={() => (transport.launchStudio())} />
            {:else if page.id == "studio"}
                <PageButton {page} {selectedPage} onClick={() => (transport.launchStudio())} />
            {:else}
                <PageButton {page} {selectedPage} onClick={() => (selectedPage = page.id)} />
            {/if}
        {/each}
    </div>

    <main class="flex-1 p-3 dark:bg-stone-950 dark:text-neutral-100 {selectedPage === 'avatar' ? 'border-r border-gray-300' : ''}">
        {#if selectedPage === "play"}
            <PlayPage {transport} onNavigate={(page) => selectedPage = page} />
        {:else if selectedPage === "avatar"}
            <AvatarPage {transport} />
        {:else if selectedPage === "levels"}
            <LevelsPage {transport} />
        {:else if selectedPage === "packages"}
            <PackagesPage {transport} />
        {:else if selectedPage === "favorites"}
            <FavoritesPage />
        {:else if selectedPage === "settings"}
            <SettingsPage {transport} />
        {:else if selectedPage === "about"}
            <AboutPage {transport} />
        {/if}
    </main>
</div>
