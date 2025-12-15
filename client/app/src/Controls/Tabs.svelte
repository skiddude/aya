<script>
    import { onMount } from "svelte"

    let { tabs = [], activeTab = $bindable(), hideTabs = false, roundedCorners = true, ontabchange = null, class: className = "", ...restProps } = $props()

    let tabRefs = {}
    let lineStyle = $state("")

    function setActiveTab(id) {
        activeTab = id
        if (ontabchange) ontabchange(id)
        updateLine()
    }

    function updateLine() {
        const activeTabRef = tabRefs[activeTab]
        if (activeTabRef) {
            lineStyle = `left: ${activeTabRef.offsetLeft}px; width: ${activeTabRef.offsetWidth}px;`
        }
    }

    function handleResize() {
        updateLine()
    }

    let tabClasses = $derived(tabs.map((tab, index) => {
        let classes = ["aya-anim-pop", "px-6", "py-2", "text-lg", "transition", "duration-200", "hover:bg-black/5", "dark:hover:bg-white/5", className]

        if (activeTab === tab.id) {
            classes.push("font-medium", "text-aya-500")
        } else {
            classes.push("text-neutral-500", "dark:text-neutral-400", "hover:text-neutral-700", "dark:hover:text-neutral-300")
        }

        if (roundedCorners) {
            classes.push("rounded-t-lg")
        } else {
            if (index === 0) classes.push("rounded-tl-lg")
            if (index === tabs.length - 1) classes.push("rounded-tr-lg")
            if (index !== 0 && index !== tabs.length - 1) classes.push("border-t-0")
        }

        return classes.join(" ")
    }))

    onMount(() => {
        updateLine()
    })

    $effect(() => {
        activeTab
        hideTabs
        updateLine()
    })
</script>

<svelte:window onresize={handleResize} />

<div class="relative mb-2 flex {className}" {...restProps}>
    {#each tabs as tab, index}
        <button bind:this={tabRefs[tab.id]} onclick={() => setActiveTab(tab.id)} class={tabClasses[index]}>
            {tab.label}
        </button>
    {/each}
    {#if !hideTabs}
        <span class="absolute bottom-0 h-0.5 rounded bg-aya-500 transition-all duration-200 ease-in-out" style={lineStyle}></span>
    {/if}
    <slot name="header" />
</div>

<slot {activeTab}></slot>