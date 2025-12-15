<script>
    let { checked = $bindable(false), onchange = null, label = null, labelClass = "", class: className = "", children, ...restProps } = $props()
    
    const handleChange = () => {
        if (onchange) onchange(checked)
    }
</script>

<label class="flex items-center">
    <input 
        type="checkbox" 
        bind:checked 
        onchange={handleChange} 
        class="aya-anim-pop cursor-pointer rounded border-gray-300 text-aya-500 shadow-sm transition duration-100 focus:ring-aya-400 dark:border-zinc-700 dark:bg-zinc-900 {className}" 
        {...restProps} 
    />
    {#if label}
        <span class="ml-2 text-gray-700 dark:text-gray-300 {labelClass}">{label}</span>
    {:else}
        <span class="ml-2 flex items-center text-gray-700 dark:text-gray-300 {labelClass}">{@render children?.()}</span>
    {/if}
</label>