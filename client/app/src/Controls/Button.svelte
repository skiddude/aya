<script>
    import { onMount } from "svelte"
    import { writable } from "svelte/store"

    let { icon = null, text = null, disabled = false, class: className = "", onclick = null, children, ...restProps } = $props()

    const iconStore = writable(icon)

    onMount(() => iconStore.set(`fa-regular ${icon}`))
    
    const handleMouseEnter = () => {
        if (!disabled) iconStore.set(`fa-solid ${icon}`)
    }
    
    const handleMouseLeave = () => {
        if (!disabled) iconStore.set(`fa-regular ${icon}`)
    }
    
    const handleMouseClick = (e) => {
        if (!disabled && onclick) onclick(e)
    }
</script>

<button 
    class="aya-anim-pop inline-flex h-10 min-h-10 items-center justify-center gap-2 rounded-lg bg-aya-500 px-3 text-lg text-white transition duration-200 hover:bg-aya-600 disabled:opacity-35 {className}" 
    onmouseenter={handleMouseEnter} 
    onmouseleave={handleMouseLeave} 
    onclick={handleMouseClick} 
    disabled={disabled || null} 
    {...restProps}
>
    {#if icon}
        <i class={$iconStore} />
    {/if}

    {#if text}
        <span>{text}</span>
    {:else}
        {@render children?.()}
    {/if}
</button>