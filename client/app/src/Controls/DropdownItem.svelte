<script>
    import { getContext } from "svelte"
    
    let { text = null, icon = null, iconPosition = "left", style = null, selected = false, onclick = null, class: className = "", ...restProps } = $props()

    const { close } = getContext("dropdown")

    function handleClick() {
        if (onclick) onclick()
        close()
    }
</script>

<button 
    onclick={handleClick} 
    class="aya-anim-pop flex w-full items-center px-3 py-1 text-sm text-neutral-700 transition duration-100 dark:text-neutral-300 {style === 'danger' ? 'hover:bg-red-100 hover:text-red-900 dark:hover:bg-red-950 dark:hover:text-red-100' : 'hover:bg-gray-100 dark:hover:bg-zinc-700'} {icon && iconPosition === 'right' ? 'justify-between' : ''} {selected ? 'bg-gray-200 hover:bg-gray-300' : ''} {className}" 
    role="menuitem"
    {...restProps}
>
    {#if icon && iconPosition === "left"}
        <i class="{selected ? 'fa-solid' : 'fa-light'} {icon} fa-fw me-1" />
    {/if}
    <span class={selected ? "font-semibold" : "font-normal"}>{text}</span>
    {#if icon && iconPosition === "right"}
        <i class="{selected ? 'fa-solid' : 'fa-light'} {icon} fa-fw" />
    {/if}
</button>